#pragma once

#include <filesystem>
#include <functional>
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
    using RecordCallback = std::function<void(
        const std::vector<std::string>& headers,
        const std::vector<std::string>& values
    )>;

    /// Lee un archivo completo y devuelve sus filas asociadas a la cabecera.
    static std::vector<Row> read(const std::filesystem::path& filePath);

    /**
     * @brief Recorre un CSV sin almacenar todas sus filas en memoria.
     *
     * Esta variante se usa con archivos masivos, como calendar.csv, para
     * procesar cada registro y descartarlo inmediatamente.
     */
    static void forEachRecord(const std::filesystem::path& filePath, const RecordCallback& callback);

    /// Divide una linea CSV respetando comillas y comillas escapadas.
    static std::vector<std::string> parseLine(const std::string& line);
};
