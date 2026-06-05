#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class CsvReader {
public:
    using Row = std::unordered_map<std::string, std::string>;

    static std::vector<Row> read(const std::filesystem::path& filePath);
    static std::vector<std::string> parseLine(const std::string& line);
};

