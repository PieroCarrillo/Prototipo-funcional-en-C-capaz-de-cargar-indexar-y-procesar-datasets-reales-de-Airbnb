#pragma once

#include "DirectoryScanner.hpp"
#include "Listing.hpp"

#include <filesystem>
#include <string>
#include <vector>

struct CalendarEntry {
    long long listingId = 0;
    std::string date;
    bool available = false;
    double price = 0.0;
    std::string sourceFile;
};

struct LoadReport {
    std::vector<CsvFileInfo> csvFiles;
    std::size_t listingFiles = 0;
    std::size_t rowsRead = 0;
    std::size_t rowsLoaded = 0;
    std::size_t rowsSkipped = 0;
};

struct CalendarReport {
    std::vector<CsvFileInfo> csvFiles;
    std::size_t calendarFiles = 0;
    std::size_t rowsRead = 0;
    std::size_t rowsLoaded = 0;
    std::size_t rowsSkipped = 0;
};

class DataLoader {
public:
    static std::vector<Listing> loadListings(const std::filesystem::path& root, LoadReport& report);
    static std::vector<CalendarEntry> loadCalendar(const std::filesystem::path& root, CalendarReport& report);
};
