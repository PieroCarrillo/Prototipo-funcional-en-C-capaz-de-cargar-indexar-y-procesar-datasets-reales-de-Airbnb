#include "AirbnbIndex.hpp"
#include "DataLoader.hpp"
#include "DirectoryScanner.hpp"

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

struct Options {
    std::filesystem::path dataRoot = "data/pilot";
    std::string query = "playa";
    long long id = 1001;
    double minPrice = 40.0;
    double maxPrice = 120.0;
    std::size_t top = 5;
    std::filesystem::path exportPath;
};

double millisecondsSince(const Clock::time_point& start) {
    const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - start);
    return elapsed.count();
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

void printListing(const Listing& listing) {
    std::cout << "  #" << listing.id << " | " << listing.name
              << " | " << listing.neighbourhood
              << " | " << listing.roomType
              << " | $" << std::fixed << std::setprecision(2) << listing.price
              << " | reviews=" << listing.numberOfReviews
              << " | score=" << std::setprecision(2) << listing.score()
              << "\n";
}

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
        const Options options = parseOptions(argc, argv);

        std::cout << "Airbnb Indexer - Segundo Avance\n";
        std::cout << "Carpeta de datos: " << options.dataRoot << "\n";

        LoadReport report;
        const auto loadStart = Clock::now();
        std::vector<Listing> loadedListings = DataLoader::loadListings(options.dataRoot, report);
        const double loadMs = millisecondsSince(loadStart);

        std::cout << "\nArchivos CSV encontrados:\n";
        for (const CsvFileInfo& file : report.csvFiles) {
            std::cout << "  [" << DirectoryScanner::kindName(file.kind) << "] " << file.path.string() << "\n";
        }

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
        std::cout << "  Tiempo de carga: " << std::fixed << std::setprecision(3) << loadMs << " ms\n";
        std::cout << "  Tiempo de indexacion: " << indexMs << " ms\n";
        std::cout << "  Memoria estimada indices: " << index.estimatedMemoryBytes() << " bytes\n";

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
