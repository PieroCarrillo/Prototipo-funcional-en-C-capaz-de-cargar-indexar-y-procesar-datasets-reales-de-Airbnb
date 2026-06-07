#include "DataLoader.hpp"

#include "CsvReader.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
std::string getValue(const CsvReader::Row& row, const std::string& key) {
    auto it = row.find(key);
    if (it != row.end()) {
        return it->second;
    }
    return "";
}

std::string cleanNumber(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char ch) {
        return ch == '$' || ch == ',' || std::isspace(ch);
    }), value.end());
    return value;
}

long long toLongLong(const std::string& value, long long fallback = 0) {
    const std::string cleaned = cleanNumber(value);
    if (cleaned.empty()) {
        return fallback;
    }
    char* end = nullptr;
    const long long parsed = std::strtoll(cleaned.c_str(), &end, 10);
    return end == cleaned.c_str() ? fallback : parsed;
}

int toInt(const std::string& value, int fallback = 0) {
    return static_cast<int>(toLongLong(value, fallback));
}

double toDouble(const std::string& value, double fallback = 0.0) {
    const std::string cleaned = cleanNumber(value);
    if (cleaned.empty()) {
        return fallback;
    }
    char* end = nullptr;
    const double parsed = std::strtod(cleaned.c_str(), &end);
    return end == cleaned.c_str() ? fallback : parsed;
}
}

std::vector<Listing> DataLoader::loadListings(const std::filesystem::path& root, LoadReport& report) {
    report = LoadReport{};
    report.csvFiles = DirectoryScanner::findCsvFiles(root);

    std::vector<Listing> listings;
    for (const CsvFileInfo& fileInfo : report.csvFiles) {
        if (fileInfo.kind != CsvKind::Listings) {
            continue;
        }

        ++report.listingFiles;
        const std::vector<CsvReader::Row> rows = CsvReader::read(fileInfo.path);
        report.rowsRead += rows.size();

        for (const CsvReader::Row& row : rows) {
            Listing listing;
            listing.id = toLongLong(getValue(row, "id"));
            listing.hostId = toLongLong(getValue(row, "host_id"));
            listing.name = getValue(row, "name");
            listing.hostName = getValue(row, "host_name");
            listing.neighbourhood = getValue(row, "neighbourhood");
            listing.roomType = getValue(row, "room_type");
            listing.price = toDouble(getValue(row, "price"));
            listing.minimumNights = toInt(getValue(row, "minimum_nights"));
            listing.numberOfReviews = toInt(getValue(row, "number_of_reviews"));
            listing.reviewsPerMonth = toDouble(getValue(row, "reviews_per_month"));
            listing.availability365 = toInt(getValue(row, "availability_365"));
            listing.latitude = toDouble(getValue(row, "latitude"));
            listing.longitude = toDouble(getValue(row, "longitude"));
            listing.sourceFile = fileInfo.path.string();

            if (listing.id == 0 || listing.name.empty()) {
                ++report.rowsSkipped;
                continue;
            }

            listings.push_back(std::move(listing));
            ++report.rowsLoaded;
        }
    }

    return listings;
}

std::vector<CalendarEntry> DataLoader::loadCalendar(const std::filesystem::path& root, CalendarReport& report) {
    report = CalendarReport{};
    report.csvFiles = DirectoryScanner::findCsvFiles(root);

    std::vector<CalendarEntry> entries;
    for (const CsvFileInfo& fileInfo : report.csvFiles) {
        if (fileInfo.kind != CsvKind::Calendar) {
            continue;
        }

        ++report.calendarFiles;
        const std::vector<CsvReader::Row> rows = CsvReader::read(fileInfo.path);
        report.rowsRead += rows.size();

        for (const CsvReader::Row& row : rows) {
            CalendarEntry entry;
            entry.listingId = toLongLong(getValue(row, "listing_id"));
            entry.date = getValue(row, "date");
            entry.available = getValue(row, "available") == "t" || getValue(row, "available") == "true";
            entry.price = toDouble(getValue(row, "price"));
            entry.sourceFile = fileInfo.path.string();

            if (entry.listingId == 0 || entry.date.empty()) {
                ++report.rowsSkipped;
                continue;
            }

            entries.push_back(std::move(entry));
            ++report.rowsLoaded;
        }
    }

    return entries;
}
