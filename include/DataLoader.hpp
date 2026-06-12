#pragma once

#include "DirectoryScanner.hpp"
#include "Listing.hpp"

#include <filesystem>
#include <string>
#include <vector>

/// Estadisticas producidas durante la carga de archivos listings.
struct LoadReport {
    std::vector<CsvFileInfo> csvFiles;
    std::size_t listingFiles = 0;
    std::size_t rowsRead = 0;
    std::size_t rowsLoaded = 0;
    std::size_t rowsSkipped = 0;
};

/// Estadisticas producidas durante la carga de archivos calendar.
struct CalendarReport {
    std::vector<CsvFileInfo> csvFiles;
    std::size_t calendarFiles = 0;
    std::size_t rowsRead = 0;
    std::size_t rowsLoaded = 0;
    std::size_t rowsSkipped = 0;
    std::size_t groupedListings = 0;
};

/**
 * @brief Convierte filas CSV en estructuras del dominio Airbnb.
 *
 * El cargador normaliza valores numericos, valida campos obligatorios y
 * registra las filas aceptadas u omitidas para facilitar la trazabilidad.
 */
class DataLoader {
public:
    /// Carga todos los archivos listings encontrados bajo la carpeta raiz.
    static std::vector<Listing> loadListings(const std::filesystem::path& root, LoadReport& report);

    /**
     * @brief Procesa todo calendar.csv y agrega sus valores por alojamiento.
     *
     * Cada fila valida se lee mediante streaming. El precio diario se usa
     * cuando existe; en caso contrario se acumula disponibilidad como 1 o 0.
     */
    static std::vector<double> loadCalendarAggregates(
        const std::filesystem::path& root,
        CalendarReport& report
    );
};
