#include "DataLoader.hpp"

#include "CsvReader.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {
// Funciones auxiliares de limpieza y conversion. Los datasets Airbnb pueden
// contener simbolos monetarios, separadores y valores vacios.
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

std::size_t columnIndex(const std::vector<std::string>& headers, const std::string& name) {
    auto it = std::find(headers.begin(), headers.end(), name);
    return it == headers.end()
        ? std::numeric_limits<std::size_t>::max()
        : static_cast<std::size_t>(it - headers.begin());
}

std::string valueAt(const std::vector<std::string>& values, std::size_t index) {
    return index < values.size() ? values[index] : "";
}
}

// Carga todos los listings detectados y descarta registros sin identidad.
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
            const std::string rawPrice = getValue(row, "price");
            listing.hasPrice = !cleanNumber(rawPrice).empty();
            listing.price = toDouble(rawPrice);
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

// Recorre todo el calendario y reduce sus filas a un valor acumulado por
// alojamiento. Esta estrategia conserva la cobertura total sin mantener
// decenas de millones de cadenas y registros completos en memoria.
std::vector<double> DataLoader::loadCalendarAggregates(
    const std::filesystem::path& root,
    CalendarReport& report
) {
    report = CalendarReport{};
    report.csvFiles = DirectoryScanner::findCsvFiles(root);

    std::unordered_map<long long, double> totalsByListing;
    for (const CsvFileInfo& fileInfo : report.csvFiles) {
        if (fileInfo.kind != CsvKind::Calendar) {
            continue;
        }

        ++report.calendarFiles;
        std::size_t listingIdIndex = std::numeric_limits<std::size_t>::max();
        std::size_t dateIndex = std::numeric_limits<std::size_t>::max();
        std::size_t availableIndex = std::numeric_limits<std::size_t>::max();
        std::size_t priceIndex = std::numeric_limits<std::size_t>::max();
        bool columnsResolved = false;

        CsvReader::forEachRecord(fileInfo.path, [&](const auto& headers, const auto& values) {
            if (!columnsResolved) {
                listingIdIndex = columnIndex(headers, "listing_id");
                dateIndex = columnIndex(headers, "date");
                availableIndex = columnIndex(headers, "available");
                priceIndex = columnIndex(headers, "price");
                columnsResolved = true;
            }

            ++report.rowsRead;
            const long long listingId = toLongLong(valueAt(values, listingIdIndex));
            const std::string date = valueAt(values, dateIndex);
            if (listingId == 0 || date.empty()) {
                ++report.rowsSkipped;
                return;
            }

            const std::string availableValue = valueAt(values, availableIndex);
            const bool available = availableValue == "t" || availableValue == "true";
            const double price = toDouble(valueAt(values, priceIndex));
            totalsByListing[listingId] += price > 0.0 ? price : (available ? 1.0 : 0.0);
            ++report.rowsLoaded;
        });
    }

    std::vector<std::pair<long long, double>> orderedTotals(
        totalsByListing.begin(),
        totalsByListing.end()
    );
    std::sort(orderedTotals.begin(), orderedTotals.end());

    std::vector<double> values;
    values.reserve(orderedTotals.size());
    for (const auto& item : orderedTotals) {
        values.push_back(item.second);
    }
    report.groupedListings = values.size();
    return values;
}
