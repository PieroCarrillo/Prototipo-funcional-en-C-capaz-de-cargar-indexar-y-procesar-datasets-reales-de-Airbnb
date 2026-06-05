#pragma once

#include <filesystem>
#include <vector>

enum class CsvKind {
    Listings,
    Reviews,
    Calendar,
    Neighbourhoods,
    Other
};

struct CsvFileInfo {
    std::filesystem::path path;
    CsvKind kind = CsvKind::Other;
};

class DirectoryScanner {
public:
    static std::vector<CsvFileInfo> findCsvFiles(const std::filesystem::path& root);
    static CsvKind classify(const std::filesystem::path& filePath);
    static const char* kindName(CsvKind kind);
};

