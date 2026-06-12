#include "AirbnbIndex.hpp"
#include "DataLoader.hpp"
#include "DirectoryScanner.hpp"
#include "GraphAnalytics.hpp"
#include "RangeAnalytics.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;

/// Parametros configurables desde la linea de comandos.
struct Options {
    std::filesystem::path dataRoot = "data/pilot";
    std::string query = "playa";
    long long id = 1001;
    double minPrice = 40.0;
    double maxPrice = 120.0;
    std::size_t top = 5;
    std::filesystem::path exportPath;
};

/// Convierte una medicion de chrono a milisegundos.
double millisecondsSince(const Clock::time_point& start) {
    const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - start);
    return elapsed.count();
}

/// Interpreta los argumentos recibidos y valida que cada opcion tenga valor.
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
            options.top = static_cast<std::size_t>(std::stoul(nextValue(arg)));
        } else if (arg == "--export") {
            options.exportPath = nextValue(arg);
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Uso: airbnb_indexer [--data carpeta] [--query texto] [--id numero]\n"
                << "                       [--min-price numero] [--max-price numero]\n"
                << "                       [--top numero] [--export archivo.csv]\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Parametro no reconocido: " + arg);
        }
    }
    return options;
}

/// Imprime un alojamiento con los campos mas relevantes para la demostracion.
void printListing(const Listing& listing) {
    std::cout << "  #" << listing.id << " | " << listing.name
              << " | " << listing.neighbourhood
              << " | " << listing.roomType
              << " | $" << std::fixed << std::setprecision(2) << listing.price
              << " | reviews=" << listing.numberOfReviews
              << " | score=" << std::setprecision(2) << listing.score()
              << "\n";
}

/// Imprime una seccion de resultados y controla el caso sin coincidencias.
void printSection(const std::string& title, const std::vector<const Listing*>& results) {
    std::cout << "\n" << title << "\n";
    if (results.empty()) {
        std::cout << "  Sin resultados.\n";
        return;
    }
    for (const Listing* listing : results) {
        printListing(*listing);
    }
}

/// Imprime las metricas comunes de los algoritmos de caminos minimos.
void printShortestPathSummary(const ShortestPathSummary& summary) {
    std::cout << "  " << summary.algorithm
              << " | nodos alcanzables=" << summary.reachableNodes
              << " | distancia total=" << std::fixed << std::setprecision(3) << summary.totalDistance << " km"
              << " | tiempo=" << summary.elapsedMs << " ms"
              << " | memoria~=" << summary.memoryBytes << " bytes\n";
}

/// Imprime costo, aristas, tiempo y memoria de un algoritmo MST.
void printMstSummary(const MstSummary& summary) {
    std::cout << "  " << summary.algorithm
              << " | aristas=" << summary.edgesUsed
              << " | costo total=" << std::fixed << std::setprecision(3) << summary.totalCost << " km"
              << " | tiempo=" << summary.elapsedMs << " ms"
              << " | memoria~=" << summary.memoryBytes << " bytes\n";
}

/// Imprime las metricas de una estructura de consulta por rango.
void printRangeMetric(const RangeMetric& metric) {
    std::cout << "  " << metric.algorithm
              << " | coincidencias=" << metric.matches
              << " | tiempo=" << std::fixed << std::setprecision(4) << metric.elapsedMs << " ms"
              << " | throughput=" << std::setprecision(2) << metric.throughputQueriesPerSecond << " consultas/s"
              << " | memoria~=" << metric.memoryBytes << " bytes\n";
}

/// Escapa campos para producir un CSV valido durante la exportacion.
std::string csvEscape(const std::string& value) {
    if (value.find_first_of(",\"\n") == std::string::npos) {
        return value;
    }

    std::string escaped = "\"";
    for (char ch : value) {
        if (ch == '"') {
            escaped += "\"\"";
        } else {
            escaped.push_back(ch);
        }
    }
    escaped.push_back('"');
    return escaped;
}

/// Exporta una coleccion de resultados a un archivo CSV reutilizable.
void exportResults(const std::filesystem::path& path, const std::vector<const Listing*>& results) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("No se pudo crear archivo de exportacion: " + path.string());
    }

    output << "id,name,neighbourhood,room_type,price,number_of_reviews,availability_365,score\n";
    for (const Listing* listing : results) {
        output << listing->id << ','
               << csvEscape(listing->name) << ','
               << csvEscape(listing->neighbourhood) << ','
               << csvEscape(listing->roomType) << ','
               << listing->price << ','
               << listing->numberOfReviews << ','
               << listing->availability365 << ','
               << listing->score() << "\n";
    }
}
}

int main(int argc, char* argv[]) {
    try {
        // Etapa 1: configuracion de la ejecucion.
        const Options options = parseOptions(argc, argv);

        std::cout << "Airbnb Indexer - Segundo Avance\n";
        std::cout << "Carpeta de datos: " << options.dataRoot << "\n";

        // Etapa 2: descubrimiento y carga de listings y calendario.
        LoadReport report;
        const auto loadStart = Clock::now();
        std::vector<Listing> loadedListings = DataLoader::loadListings(options.dataRoot, report);
        const double loadMs = millisecondsSince(loadStart);

        CalendarReport calendarReport;
        std::vector<CalendarEntry> calendarEntries = DataLoader::loadCalendar(options.dataRoot, calendarReport);

        std::cout << "\nArchivos CSV encontrados:\n";
        for (const CsvFileInfo& file : report.csvFiles) {
            std::cout << "  [" << DirectoryScanner::kindName(file.kind) << "] " << file.path.string() << "\n";
        }

        // Etapa 3: construccion de indices residentes en memoria RAM.
        AirbnbIndex index;
        for (Listing& listing : loadedListings) {
            index.add(std::move(listing));
        }

        const auto indexStart = Clock::now();
        index.build();
        const double indexMs = millisecondsSince(indexStart);

        std::cout << "\nResumen de carga\n";
        std::cout << "  Archivos listings: " << report.listingFiles << "\n";
        std::cout << "  Filas leidas: " << report.rowsRead << "\n";
        std::cout << "  Listings cargados: " << report.rowsLoaded << "\n";
        std::cout << "  Filas omitidas: " << report.rowsSkipped << "\n";
        std::cout << "  Filas calendar cargadas: " << calendarReport.rowsLoaded << "\n";
        std::cout << "  Tiempo de carga: " << std::fixed << std::setprecision(3) << loadMs << " ms\n";
        std::cout << "  Tiempo de indexacion: " << indexMs << " ms\n";
        std::cout << "  Memoria estimada indices: " << index.estimatedMemoryBytes() << " bytes\n";

        // Etapa 4: busquedas exactas, textuales, filtros y ranking.
        const auto exactStart = Clock::now();
        const Listing* exact = index.findById(options.id);
        const double exactMs = millisecondsSince(exactStart);

        std::cout << "\nBusqueda exacta por ID " << options.id << " (" << exactMs << " ms)\n";
        if (exact) {
            printListing(*exact);
        } else {
            std::cout << "  No encontrado.\n";
        }

        const auto textStart = Clock::now();
        const auto textResults = index.searchText(options.query, options.top);
        const double textMs = millisecondsSince(textStart);
        printSection("Busqueda parcial: \"" + options.query + "\" (" + std::to_string(textMs) + " ms)", textResults);

        const auto filterStart = Clock::now();
        const auto priceResults = index.filterByPrice(options.minPrice, options.maxPrice, options.top);
        const double filterMs = millisecondsSince(filterStart);
        printSection(
            "Filtro por precio $" + std::to_string(options.minPrice) + " - $" + std::to_string(options.maxPrice) +
                " (" + std::to_string(filterMs) + " ms)",
            priceResults
        );

        const auto sortStart = Clock::now();
        const auto cheapest = index.sortByPrice(true, options.top);
        const double sortMs = millisecondsSince(sortStart);
        printSection("Ordenamiento por precio ascendente (" + std::to_string(sortMs) + " ms)", cheapest);

        const auto rankStart = Clock::now();
        const auto ranked = index.topRanked(options.top);
        const double rankMs = millisecondsSince(rankStart);
        printSection("Ranking por score con priority_queue (" + std::to_string(rankMs) + " ms)", ranked);

        // Etapa 5 - Modulo M4: caminos minimos entre alojamientos.
        std::cout << "\nM4 - Busqueda en grafos sobre proximidad geografica\n";
        WeightedGraph listingGraph = GraphAnalytics::buildListingProximityGraph(index.listings(), 3);
        std::cout << "  Grafo de listings: nodos=" << listingGraph.labels.size()
                  << " | aristas=" << listingGraph.edges.size() << "\n";
        if (!listingGraph.labels.empty()) {
            printShortestPathSummary(GraphAnalytics::runDijkstra(listingGraph, 0));
            printShortestPathSummary(GraphAnalytics::runBellmanFord(listingGraph, 0));
            printShortestPathSummary(GraphAnalytics::runFloydWarshall(listingGraph));
        }

        // Etapa 6 - Modulo M4: MST, DFS y Tarjan sobre barrios.
        std::cout << "\nM4 - MST y recorridos sobre grafo de barrios\n";
        WeightedGraph neighbourhoodGraph = GraphAnalytics::buildNeighbourhoodGraph(index.listings());
        std::cout << "  Grafo de barrios: nodos=" << neighbourhoodGraph.labels.size()
                  << " | aristas=" << neighbourhoodGraph.edges.size() << "\n";
        if (!neighbourhoodGraph.labels.empty()) {
            printMstSummary(GraphAnalytics::runKruskal(neighbourhoodGraph));
            printMstSummary(GraphAnalytics::runPrim(neighbourhoodGraph));
            printMstSummary(GraphAnalytics::runBoruvka(neighbourhoodGraph));
            TraversalSummary traversal = GraphAnalytics::runDfsAndTarjan(neighbourhoodGraph);
            std::cout << "  DFS | visitados=" << traversal.dfsVisited
                      << " | tiempo=" << traversal.dfsElapsedMs << " ms"
                      << " | nodos/s=" << traversal.nodesPerSecond << "\n";
            std::cout << "  Tarjan | puntos de articulacion=" << traversal.articulationPoints.size()
                      << " | tiempo=" << traversal.tarjanElapsedMs << " ms\n";
        }

        // Etapa 7 - Modulo M5: comparacion de consultas por rango de precio.
        std::cout << "\nM5 - Busqueda por rangos de precio\n";
        for (const RangeMetric& metric : RangeAnalytics::comparePriceRange(
                 index.listings(),
                 options.minPrice,
                 options.maxPrice
             )) {
            printRangeMetric(metric);
        }

        // Etapa 8 - Modulo M5: actualizaciones dinamicas del calendario.
        std::cout << "\nM5 - Simulacion de calendario dinamico\n";
        CalendarSimulationResult calendarResult = RangeAnalytics::simulateCalendarUpdates(calendarEntries);
        std::cout << "  Entradas calendar=" << calendarResult.entries << "\n";
        std::cout << "  Segment Tree + lazy | suma=" << calendarResult.segmentTreeSum
                  << " | tiempo=" << calendarResult.segmentTreeLazyMs << " ms\n";
        std::cout << "  Fenwick puntual | suma=" << calendarResult.fenwickSum
                  << " | tiempo=" << calendarResult.fenwickPointUpdateMs << " ms\n";
        std::cout << "  AVL | suma=" << calendarResult.avlSum
                  << " | tiempo=" << calendarResult.avlUpdateQueryMs << " ms\n";
        std::cout << "  Memoria dinamica estimada=" << calendarResult.memoryBytes << " bytes\n";

        // Etapa 9: exportacion opcional del ranking.
        if (!options.exportPath.empty()) {
            exportResults(options.exportPath, ranked);
            std::cout << "\nResultados exportados a: " << options.exportPath << "\n";
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
