#include "AirbnbIndex.hpp"
#include "DataLoader.hpp"
#include "DirectoryScanner.hpp"
#include "GraphAnalytics.hpp"
#include "RangeAnalytics.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
constexpr std::size_t MaxFloydWarshallListings = 500;

/// Parametros iniciales configurables desde la linea de comandos.
struct Options {
    std::filesystem::path dataRoot = "data/pilot";
    std::string query = "playa";
    long long id = 1001;
    double minPrice = 40.0;
    double maxPrice = 120.0;
    std::size_t top = 5;
    std::size_t graphLimit = 100;
    std::filesystem::path exportPath = "exports/ranking.csv";
};

/// Estado compartido por los cinco modulos durante la sesion interactiva.
struct ApplicationState {
    Options options;
    LoadReport loadReport;
    AirbnbIndex index;
    double loadMs = 0.0;
    double indexMs = 0.0;
    CalendarReport calendarReport;
    std::vector<double> calendarValues;
    double calendarLoadMs = 0.0;
    bool calendarLoaded = false;
};

double millisecondsSince(const Clock::time_point& start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

Options parseOptions(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto nextValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Falta valor para " + name);
            }
            return argv[++i];
        };

        if (arg == "--data") {
            options.dataRoot = nextValue(arg);
        } else if (arg == "--query") {
            options.query = nextValue(arg);
        } else if (arg == "--id") {
            options.id = std::stoll(nextValue(arg));
        } else if (arg == "--min-price") {
            options.minPrice = std::stod(nextValue(arg));
        } else if (arg == "--max-price") {
            options.maxPrice = std::stod(nextValue(arg));
        } else if (arg == "--top") {
            options.top = static_cast<std::size_t>(std::stoull(nextValue(arg)));
        } else if (arg == "--graph-limit") {
            options.graphLimit = static_cast<std::size_t>(std::stoull(nextValue(arg)));
        } else if (arg == "--export") {
            options.exportPath = nextValue(arg);
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Uso: airbnb_indexer [--data carpeta] [--query texto] [--id numero]\n"
                << "                       [--min-price numero] [--max-price numero]\n"
                << "                       [--top numero] [--graph-limit numero]\n"
                << "                       [--export archivo.csv]\n\n"
                << "La aplicacion abre un menu interactivo organizado en M1, M2, M3, M4 y M5.\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Parametro no reconocido: " + arg);
        }
    }
    return options;
}

std::string readText(const std::string& prompt, const std::string& defaultValue = "") {
    std::cout << prompt;
    if (!defaultValue.empty()) {
        std::cout << " [" << defaultValue << "]";
    }
    std::cout << ": ";

    std::string value;
    if (!std::getline(std::cin, value)) {
        return defaultValue;
    }
    return value.empty() ? defaultValue : value;
}

template <typename T>
T readNumber(const std::string& prompt, T defaultValue) {
    while (true) {
        const std::string text = readText(prompt, std::to_string(defaultValue));
        std::istringstream input(text);
        T value{};
        char remaining = '\0';
        if (input >> value && !(input >> remaining)) {
            return value;
        }
        std::cout << "Entrada no valida. Intente nuevamente.\n";
    }
}

int readChoice(const std::string& prompt, int minValue, int maxValue) {
    while (true) {
        const int choice = readNumber<int>(prompt, 0);
        if (choice >= minValue && choice <= maxValue) {
            return choice;
        }
        std::cout << "Seleccione una opcion entre " << minValue << " y " << maxValue << ".\n";
    }
}

void waitForEnter() {
    std::cout << "\nPresione ENTER para volver al menu...";
    std::string ignored;
    std::getline(std::cin, ignored);
}

void printHeader(const std::string& title) {
    std::cout << "\n============================================================\n";
    std::cout << title << "\n";
    std::cout << "============================================================\n";
}

void printListing(const Listing& listing) {
    std::cout << "  #" << listing.id << " | " << listing.name
              << " | " << listing.neighbourhood
              << " | " << listing.roomType;
    if (listing.hasPrice) {
        std::cout << " | $" << std::fixed << std::setprecision(2) << listing.price;
    } else {
        std::cout << " | precio=N/D";
    }
    std::cout << " | reviews=" << listing.numberOfReviews
              << " | score=" << std::fixed << std::setprecision(2) << listing.score()
              << "\n";
}

void printResults(const std::string& title, const std::vector<const Listing*>& results, double elapsedMs) {
    std::cout << "\n" << title << " (" << std::fixed << std::setprecision(4)
              << elapsedMs << " ms)\n";
    if (results.empty()) {
        std::cout << "  Sin resultados.\n";
        return;
    }
    for (const Listing* listing : results) {
        printListing(*listing);
    }
}

void printShortestPathSummary(const ShortestPathSummary& summary) {
    std::cout << "  " << summary.algorithm
              << " | nodos alcanzables=" << summary.reachableNodes
              << " | distancia total=" << std::fixed << std::setprecision(3)
              << summary.totalDistance << " km"
              << " | tiempo=" << summary.elapsedMs << " ms"
              << " | memoria~=" << summary.memoryBytes << " bytes\n";
}

void printMstSummary(const MstSummary& summary) {
    std::cout << "  " << summary.algorithm
              << " | aristas=" << summary.edgesUsed
              << " | costo total=" << std::fixed << std::setprecision(3)
              << summary.totalCost << " km"
              << " | tiempo=" << summary.elapsedMs << " ms"
              << " | memoria~=" << summary.memoryBytes << " bytes\n";
}

void printRangeMetric(const RangeMetric& metric) {
    std::cout << "  " << metric.algorithm
              << " | coincidencias=" << metric.matches
              << " | tiempo=" << std::fixed << std::setprecision(4)
              << metric.elapsedMs << " ms"
              << " | throughput=" << std::setprecision(2)
              << metric.throughputQueriesPerSecond << " consultas/s"
              << " | memoria~=" << metric.memoryBytes << " bytes\n";
}

std::string csvEscape(const std::string& value) {
    if (value.find_first_of(",\"\n") == std::string::npos) {
        return value;
    }

    std::string escaped = "\"";
    for (char ch : value) {
        escaped += ch == '"' ? "\"\"" : std::string(1, ch);
    }
    escaped.push_back('"');
    return escaped;
}

void exportResults(const std::filesystem::path& path, const std::vector<const Listing*>& results) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("No se pudo crear archivo de exportacion: " + path.string());
    }

    output << "id,name,neighbourhood,room_type,price,number_of_reviews,availability_365,score\n";
    for (const Listing* listing : results) {
        output << listing->id << ','
               << csvEscape(listing->name) << ','
               << csvEscape(listing->neighbourhood) << ','
               << csvEscape(listing->roomType) << ',';
        if (listing->hasPrice) {
            output << listing->price;
        }
        output << ',' << listing->numberOfReviews << ','
               << listing->availability365 << ','
               << listing->score() << "\n";
    }
}

ApplicationState initializeApplication(const Options& options) {
    ApplicationState state;
    state.options = options;

    const auto loadStart = Clock::now();
    std::vector<Listing> listings = DataLoader::loadListings(options.dataRoot, state.loadReport);
    state.loadMs = millisecondsSince(loadStart);

    for (Listing& listing : listings) {
        state.index.add(std::move(listing));
    }

    const auto indexStart = Clock::now();
    state.index.build();
    state.indexMs = millisecondsSince(indexStart);
    return state;
}

void runModuleM1(const ApplicationState& state) {
    printHeader("M1 - RECORRIDO DE DIRECTORIOS Y CARGA DE DATOS");
    std::cout << "Carpeta analizada: " << state.options.dataRoot << "\n\n";
    std::cout << "Archivos CSV encontrados:\n";
    for (const CsvFileInfo& file : state.loadReport.csvFiles) {
        std::cout << "  [" << DirectoryScanner::kindName(file.kind) << "] "
                  << file.path.string() << "\n";
    }

    std::cout << "\nResumen de listings:\n";
    std::cout << "  Archivos listings: " << state.loadReport.listingFiles << "\n";
    std::cout << "  Filas leidas: " << state.loadReport.rowsRead << "\n";
    std::cout << "  Listings cargados: " << state.loadReport.rowsLoaded << "\n";
    std::cout << "  Filas omitidas: " << state.loadReport.rowsSkipped << "\n";
    std::cout << "  Tiempo de carga: " << std::fixed << std::setprecision(3)
              << state.loadMs << " ms\n";

    if (state.calendarLoaded) {
        std::cout << "\nCalendario completo procesado:\n";
        std::cout << "  Filas leidas: " << state.calendarReport.rowsRead << "\n";
        std::cout << "  Filas validas: " << state.calendarReport.rowsLoaded << "\n";
        std::cout << "  Alojamientos agrupados: " << state.calendarReport.groupedListings << "\n";
    } else {
        std::cout << "\nEl calendario se procesa bajo demanda desde M5.\n";
    }
}

void runModuleM2(const ApplicationState& state) {
    printHeader("M2 - CONSTRUCCION DE INDICES EN MEMORIA");
    std::cout << "Coleccion base (vector): " << state.index.size() << " listings\n";
    std::cout << "Indice exacto por ID: unordered_map\n";
    std::cout << "Indice textual invertido: unordered_map de tokens\n";
    std::cout << "Indice por barrio: unordered_map\n";
    std::cout << "Indice ordenado de precios: multimap\n";
    std::cout << "Tiempo de indexacion: " << std::fixed << std::setprecision(3)
              << state.indexMs << " ms\n";
    std::cout << "Memoria estimada: " << state.index.estimatedMemoryBytes() << " bytes\n";
}

void runExactSearch(ApplicationState& state) {
    state.options.id = readNumber<long long>("ID del alojamiento", state.options.id);
    const auto start = Clock::now();
    const Listing* result = state.index.findById(state.options.id);
    const double elapsed = millisecondsSince(start);

    std::cout << "\nBusqueda exacta (" << std::fixed << std::setprecision(4)
              << elapsed << " ms)\n";
    if (result) {
        printListing(*result);
    } else {
        std::cout << "  No encontrado.\n";
    }
}

void runTextSearch(ApplicationState& state) {
    state.options.query = readText("Texto a buscar", state.options.query);
    state.options.top = readNumber<std::size_t>("Cantidad maxima de resultados", state.options.top);
    const auto start = Clock::now();
    const auto results = state.index.searchText(state.options.query, state.options.top);
    printResults("Busqueda parcial: \"" + state.options.query + "\"", results, millisecondsSince(start));
}

void readPriceRange(ApplicationState& state) {
    state.options.minPrice = readNumber<double>("Precio minimo", state.options.minPrice);
    state.options.maxPrice = readNumber<double>("Precio maximo", state.options.maxPrice);
    if (state.options.minPrice > state.options.maxPrice) {
        std::swap(state.options.minPrice, state.options.maxPrice);
    }
}

void runPriceFilter(ApplicationState& state) {
    readPriceRange(state);
    state.options.top = readNumber<std::size_t>("Cantidad maxima de resultados", state.options.top);
    const auto start = Clock::now();
    const auto results = state.index.filterByPrice(
        state.options.minPrice,
        state.options.maxPrice,
        state.options.top
    );
    printResults("Filtro por rango de precio", results, millisecondsSince(start));
}

void runPriceOrdering(ApplicationState& state) {
    state.options.top = readNumber<std::size_t>("Cantidad maxima de resultados", state.options.top);
    const int direction = readChoice("1. Ascendente | 2. Descendente", 1, 2);
    const auto start = Clock::now();
    const auto results = state.index.sortByPrice(direction == 1, state.options.top);
    printResults("Ordenamiento por precio", results, millisecondsSince(start));
}

void runReviewOrdering(ApplicationState& state) {
    state.options.top = readNumber<std::size_t>("Cantidad maxima de resultados", state.options.top);
    const auto start = Clock::now();
    const auto results = state.index.sortByReviews(state.options.top);
    printResults("Ordenamiento por cantidad de resenas", results, millisecondsSince(start));
}

void runRanking(ApplicationState& state, bool exportToFile) {
    state.options.top = readNumber<std::size_t>("Cantidad maxima de resultados", state.options.top);
    const auto start = Clock::now();
    const auto results = state.index.topRanked(state.options.top);
    printResults("Ranking por score con priority_queue", results, millisecondsSince(start));

    if (exportToFile) {
        state.options.exportPath = readText(
            "Ruta del archivo CSV",
            state.options.exportPath.string()
        );
        exportResults(state.options.exportPath, results);
        std::cout << "\nResultados exportados a: " << state.options.exportPath << "\n";
    }
}

void runModuleM3(ApplicationState& state) {
    while (true) {
        printHeader("M3 - BUSQUEDAS, FILTROS, ORDENAMIENTOS Y RANKING");
        std::cout << "1. Busqueda exacta por ID\n";
        std::cout << "2. Busqueda textual parcial\n";
        std::cout << "3. Filtro por rango de precio\n";
        std::cout << "4. Ordenamiento por precio\n";
        std::cout << "5. Ordenamiento por cantidad de resenas\n";
        std::cout << "6. Ranking por score\n";
        std::cout << "7. Exportar ranking a CSV\n";
        std::cout << "0. Volver al menu principal\n";

        const int choice = readChoice("Opcion", 0, 7);
        if (choice == 0) {
            return;
        }

        switch (choice) {
            case 1:
                runExactSearch(state);
                break;
            case 2:
                runTextSearch(state);
                break;
            case 3:
                runPriceFilter(state);
                break;
            case 4:
                runPriceOrdering(state);
                break;
            case 5:
                runReviewOrdering(state);
                break;
            case 6:
                runRanking(state, false);
                break;
            case 7:
                runRanking(state, true);
                break;
        }
        waitForEnter();
    }
}

void runModuleM4(ApplicationState& state) {
    printHeader("M4 - BUSQUEDA Y ANALISIS EN GRAFOS");
    if (state.index.listings().empty()) {
        std::cout << "No existen listings para construir grafos.\n";
        return;
    }

    std::size_t requestedLimit = readNumber<std::size_t>(
        "Listings para caminos minimos (Floyd-Warshall es O(V^3))",
        state.options.graphLimit
    );
    requestedLimit = std::max<std::size_t>(1, requestedLimit);
    if (requestedLimit > MaxFloydWarshallListings) {
        std::cout << "Por seguridad, Floyd-Warshall se limita a "
                  << MaxFloydWarshallListings << " listings.\n";
        requestedLimit = MaxFloydWarshallListings;
    }
    state.options.graphLimit = requestedLimit;

    const std::size_t graphSize = std::min(requestedLimit, state.index.listings().size());
    std::vector<Listing> graphListings(
        state.index.listings().begin(),
        state.index.listings().begin() + graphSize
    );

    std::cout << "\nCaminos minimos sobre " << graphSize << " de "
              << state.index.listings().size() << " listings:\n";
    WeightedGraph listingGraph = GraphAnalytics::buildListingProximityGraph(graphListings, 3);
    std::cout << "  Nodos=" << listingGraph.labels.size()
              << " | aristas=" << listingGraph.edges.size() << "\n";
    if (!listingGraph.labels.empty()) {
        printShortestPathSummary(GraphAnalytics::runDijkstra(listingGraph, 0));
        printShortestPathSummary(GraphAnalytics::runBellmanFord(listingGraph, 0));
        printShortestPathSummary(GraphAnalytics::runFloydWarshall(listingGraph));
    }

    std::cout << "\nMST y recorridos sobre todos los barrios detectados:\n";
    WeightedGraph neighbourhoodGraph = GraphAnalytics::buildNeighbourhoodGraph(state.index.listings());
    std::cout << "  Nodos=" << neighbourhoodGraph.labels.size()
              << " | aristas=" << neighbourhoodGraph.edges.size() << "\n";
    if (!neighbourhoodGraph.labels.empty()) {
        printMstSummary(GraphAnalytics::runKruskal(neighbourhoodGraph));
        printMstSummary(GraphAnalytics::runPrim(neighbourhoodGraph));
        printMstSummary(GraphAnalytics::runBoruvka(neighbourhoodGraph));

        const TraversalSummary traversal = GraphAnalytics::runDfsAndTarjan(neighbourhoodGraph);
        std::cout << "  DFS | visitados=" << traversal.dfsVisited
                  << " | tiempo=" << traversal.dfsElapsedMs << " ms"
                  << " | nodos/s=" << traversal.nodesPerSecond << "\n";
        std::cout << "  Tarjan | puntos de articulacion="
                  << traversal.articulationPoints.size()
                  << " | tiempo=" << traversal.tarjanElapsedMs << " ms\n";
    }
}

void runPriceRangeComparison(ApplicationState& state) {
    readPriceRange(state);
    std::cout << "\nComparacion de estructuras para el rango $"
              << std::fixed << std::setprecision(2) << state.options.minPrice
              << " - $" << state.options.maxPrice << ":\n";
    for (const RangeMetric& metric : RangeAnalytics::comparePriceRange(
             state.index.listings(),
             state.options.minPrice,
             state.options.maxPrice
         )) {
        printRangeMetric(metric);
    }
}

void ensureCalendarLoaded(ApplicationState& state) {
    if (state.calendarLoaded) {
        return;
    }

    std::cout << "\nProcesando todos los registros de calendar.csv...\n";
    std::cout << "Esta operacion puede tardar con la coleccion completa de Paris.\n";
    const auto start = Clock::now();
    state.calendarValues = DataLoader::loadCalendarAggregates(
        state.options.dataRoot,
        state.calendarReport
    );
    state.calendarLoadMs = millisecondsSince(start);
    state.calendarLoaded = true;
}

void runCalendarAnalysis(ApplicationState& state) {
    ensureCalendarLoaded(state);
    const CalendarSimulationResult result = RangeAnalytics::simulateCalendarUpdates(
        state.calendarValues,
        state.calendarReport.rowsLoaded
    );

    std::cout << "\nCalendario completo:\n";
    std::cout << "  Archivos calendar: " << state.calendarReport.calendarFiles << "\n";
    std::cout << "  Filas leidas: " << state.calendarReport.rowsRead << "\n";
    std::cout << "  Filas validas: " << result.sourceRows << "\n";
    std::cout << "  Filas omitidas: " << state.calendarReport.rowsSkipped << "\n";
    std::cout << "  Alojamientos agregados: " << result.aggregatedValues << "\n";
    std::cout << "  Tiempo de lectura y agregacion: " << std::fixed
              << std::setprecision(3) << state.calendarLoadMs << " ms\n";
    std::cout << "  Segment Tree + lazy | suma=" << result.segmentTreeSum
              << " | tiempo=" << result.segmentTreeLazyMs << " ms\n";
    std::cout << "  Fenwick puntual | suma=" << result.fenwickSum
              << " | tiempo=" << result.fenwickPointUpdateMs << " ms\n";
    std::cout << "  AVL | suma=" << result.avlSum
              << " | tiempo=" << result.avlUpdateQueryMs << " ms\n";
    std::cout << "  Memoria dinamica estimada=" << result.memoryBytes << " bytes\n";
}

void runModuleM5(ApplicationState& state) {
    while (true) {
        printHeader("M5 - BUSQUEDA POR RANGOS Y CALENDARIO DINAMICO");
        std::cout << "1. Comparar estructuras para rango de precio\n";
        std::cout << "2. Procesar calendario completo y simular actualizaciones\n";
        std::cout << "3. Ejecutar ambas operaciones\n";
        std::cout << "0. Volver al menu principal\n";

        const int choice = readChoice("Opcion", 0, 3);
        if (choice == 0) {
            return;
        }
        if (choice == 1 || choice == 3) {
            runPriceRangeComparison(state);
        }
        if (choice == 2 || choice == 3) {
            runCalendarAnalysis(state);
        }
        waitForEnter();
    }
}

void runInteractiveMenu(ApplicationState& state) {
    while (true) {
        printHeader("AIRBNB INDEXER - MENU INTERACTIVO M1 A M5");
        std::cout << "Dataset: " << state.options.dataRoot << "\n";
        std::cout << "Listings disponibles: " << state.index.size() << "\n\n";
        std::cout << "1. M1 - Recorrido de directorios y carga de datos\n";
        std::cout << "2. M2 - Construccion de indices en memoria\n";
        std::cout << "3. M3 - Busquedas, filtros, ordenamientos y ranking\n";
        std::cout << "4. M4 - Busqueda y analisis en grafos\n";
        std::cout << "5. M5 - Rangos y calendario dinamico\n";
        std::cout << "0. Salir\n";

        const int choice = readChoice("Seleccione un modulo", 0, 5);
        if (choice == 0) {
            std::cout << "\nPrograma finalizado.\n";
            return;
        }

        switch (choice) {
            case 1:
                runModuleM1(state);
                break;
            case 2:
                runModuleM2(state);
                break;
            case 3:
                runModuleM3(state);
                continue;
            case 4:
                runModuleM4(state);
                break;
            case 5:
                runModuleM5(state);
                continue;
        }
        waitForEnter();
    }
}
}

int main(int argc, char* argv[]) {
    try {
        const Options options = parseOptions(argc, argv);
        std::cout << "Preparando listings e indices desde " << options.dataRoot << "...\n";
        ApplicationState state = initializeApplication(options);
        runInteractiveMenu(state);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
