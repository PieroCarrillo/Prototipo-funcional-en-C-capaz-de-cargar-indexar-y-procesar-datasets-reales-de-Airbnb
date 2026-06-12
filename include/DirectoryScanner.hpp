#pragma once

#include <filesystem>
#include <vector>

/// Tipos de archivos reconocidos dentro de una coleccion Airbnb.
enum class CsvKind {
    Listings,
    Reviews,
    Calendar,
    Neighbourhoods,
    Other
};

/// Describe la ruta de un CSV y la categoria detectada por su nombre.
struct CsvFileInfo {
    std::filesystem::path path;
    CsvKind kind = CsvKind::Other;
};

/**
 * @brief Localiza y clasifica archivos CSV dentro de un arbol de directorios.
 *
 * Este modulo permite trabajar con datasets organizados por ciudad sin
 * especificar manualmente la ruta de cada archivo.
 */
class DirectoryScanner {
public:
    /// Recorre recursivamente la carpeta raiz y devuelve todos los CSV.
    static std::vector<CsvFileInfo> findCsvFiles(const std::filesystem::path& root);

    /// Determina el tipo de CSV a partir de su nombre de archivo.
    static CsvKind classify(const std::filesystem::path& filePath);

    /// Convierte la categoria interna en una etiqueta legible.
    static const char* kindName(CsvKind kind);
};
