#include "CsvReader.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

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

std::vector<CsvReader::Row> CsvReader::read(const std::filesystem::path& filePath) {
    std::ifstream input(filePath);
    if (!input) {
        throw std::runtime_error("No se pudo abrir el archivo CSV: " + filePath.string());
    }

    std::string headerLine;
    if (!std::getline(input, headerLine)) {
        return {};
    }

    std::vector<std::string> headers = parseLine(headerLine);
    std::vector<Row> rows;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::vector<std::string> values = parseLine(line);
        Row row;
        for (std::size_t i = 0; i < headers.size(); ++i) {
            row[headers[i]] = i < values.size() ? values[i] : "";
        }
        rows.push_back(std::move(row));
    }

    return rows;
}
