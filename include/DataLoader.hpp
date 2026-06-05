#pragma once

#include "DirectoryScanner.hpp"
#include "Listing.hpp"

#include <filesystem>
#include <vector>

struct LoadReport {
    std::vector<CsvFileInfo> csvFiles;
    std::size_t listingFiles = 0;
    std::size_t rowsRead = 0;
    std::size_t rowsLoaded = 0;
    std::size_t rowsSkipped = 0;
};

class DataLoader {
public:
    static std::vector<Listing> loadListings(const std::filesystem::path& root, LoadReport& report);
};

