#include "DirectoryScanner.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace {
// Normaliza nombres y extensiones para clasificarlos sin depender de mayusculas.
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}
}

// El recorrido recursivo permite procesar colecciones separadas por ciudad.
std::vector<CsvFileInfo> DirectoryScanner::findCsvFiles(const std::filesystem::path& root) {
    std::vector<CsvFileInfo> files;
    if (!std::filesystem::exists(root)) {
        return files;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto extension = toLower(entry.path().extension().string());
        if (extension == ".csv") {
            files.push_back({entry.path(), classify(entry.path())});
        }
    }

    std::sort(files.begin(), files.end(), [](const CsvFileInfo& left, const CsvFileInfo& right) {
        return left.path.string() < right.path.string();
    });
    return files;
}

// La clasificacion por nombre evita abrir el archivo para determinar su tipo.
CsvKind DirectoryScanner::classify(const std::filesystem::path& filePath) {
    const std::string filename = toLower(filePath.filename().string());
    if (filename.find("listings") != std::string::npos) {
        return CsvKind::Listings;
    }
    if (filename.find("reviews") != std::string::npos) {
        return CsvKind::Reviews;
    }
    if (filename.find("calendar") != std::string::npos) {
        return CsvKind::Calendar;
    }
    if (filename.find("neighbourhood") != std::string::npos) {
        return CsvKind::Neighbourhoods;
    }
    return CsvKind::Other;
}

const char* DirectoryScanner::kindName(CsvKind kind) {
    switch (kind) {
        case CsvKind::Listings:
            return "listings";
        case CsvKind::Reviews:
            return "reviews";
        case CsvKind::Calendar:
            return "calendar";
        case CsvKind::Neighbourhoods:
            return "neighbourhoods";
        default:
            return "other";
    }
}
