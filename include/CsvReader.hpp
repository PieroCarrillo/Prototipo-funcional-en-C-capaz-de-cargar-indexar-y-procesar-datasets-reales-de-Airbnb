#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Lector basico de archivos CSV con soporte para campos entre comillas.
 *
 * Cada fila se expone como un mapa columna-valor para desacoplar el cargador
 * del orden fisico de las columnas del dataset.
 */
class CsvReader {
public:
    using Row = std::unordered_map<std::string, std::string>;

    /// Lee un archivo completo y devuelve sus filas asociadas a la cabecera.
    static std::vector<Row> read(const std::filesystem::path& filePath);

    /// Divide una linea CSV respetando comillas y comillas escapadas.
    static std::vector<std::string> parseLine(const std::string& line);
};
