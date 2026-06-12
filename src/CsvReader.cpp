#include "CsvReader.hpp"

#include <fstream>
#include <istream>
#include <stdexcept>
#include <utility>

namespace {
/**
 * Lee un registro CSV logico. Un registro puede ocupar varias lineas fisicas
 * cuando un campo entre comillas contiene saltos de linea.
 */
bool readRecord(std::istream& input, std::vector<std::string>& values) {
    values.clear();
    std::string current;
    bool insideQuotes = false;
    bool readAnyCharacter = false;
    char ch = '\0';

    while (input.get(ch)) {
        readAnyCharacter = true;

        if (ch == '"') {
            if (insideQuotes && input.peek() == '"') {
                input.get(ch);
                current.push_back('"');
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (ch == ',' && !insideQuotes) {
            values.push_back(current);
            current.clear();
        } else if ((ch == '\n' || ch == '\r') && !insideQuotes) {
            if (ch == '\r' && input.peek() == '\n') {
                input.get(ch);
            }
            values.push_back(current);
            return true;
        } else {
            current.push_back(ch);
        }
    }

    if (readAnyCharacter) {
        values.push_back(current);
    }
    return readAnyCharacter;
}
}

// Analiza una linea caracter por caracter para no separar comas que formen
// parte de un campo encerrado entre comillas.
std::vector<std::string> CsvReader::parseLine(const std::string& line) {
    std::vector<std::string> values;
    std::string current;
    bool insideQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (insideQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (ch == ',' && !insideQuotes) {
            values.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    values.push_back(current);
    return values;
}

// La primera fila define los nombres de columna. Las filas posteriores se
// convierten en mapas para que otros modulos consulten campos por nombre.
std::vector<CsvReader::Row> CsvReader::read(const std::filesystem::path& filePath) {
    std::ifstream input(filePath);
    if (!input) {
        throw std::runtime_error("No se pudo abrir el archivo CSV: " + filePath.string());
    }

    std::vector<std::string> headers;
    if (!readRecord(input, headers)) {
        return {};
    }
    if (!headers.empty() && headers.front().size() >= 3 &&
        static_cast<unsigned char>(headers.front()[0]) == 0xEF &&
        static_cast<unsigned char>(headers.front()[1]) == 0xBB &&
        static_cast<unsigned char>(headers.front()[2]) == 0xBF) {
        headers.front().erase(0, 3);
    }

    std::vector<Row> rows;
    std::vector<std::string> values;
    while (readRecord(input, values)) {
        if (values.size() == 1 && values.front().empty()) {
            continue;
        }

        Row row;
        for (std::size_t i = 0; i < headers.size(); ++i) {
            row[headers[i]] = i < values.size() ? values[i] : "";
        }
        rows.push_back(std::move(row));
    }

    return rows;
}
