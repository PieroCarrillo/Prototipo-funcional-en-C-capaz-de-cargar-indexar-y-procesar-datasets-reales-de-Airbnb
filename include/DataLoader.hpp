#pragma once

#include "DirectoryScanner.hpp"
#include "Listing.hpp"

#include <filesystem>
#include <string>
#include <vector>

/// Registro diario de precio y disponibilidad de un alojamiento.
struct CalendarEntry {
    long long listingId = 0;
    std::string date;
    bool available = false;
    double price = 0.0;
    std::string sourceFile;
};

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

    /// Carga todos los archivos calendar encontrados bajo la carpeta raiz.
    static std::vector<CalendarEntry> loadCalendar(const std::filesystem::path& root, CalendarReport& report);
};
