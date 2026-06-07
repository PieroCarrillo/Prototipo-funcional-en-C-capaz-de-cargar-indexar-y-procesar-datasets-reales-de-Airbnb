#pragma once

#include "DataLoader.hpp"
#include "Listing.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct RangeMetric {
    std::string algorithm;
    std::size_t matches = 0;
    double elapsedMs = 0.0;
    double throughputQueriesPerSecond = 0.0;
    std::size_t memoryBytes = 0;
};

struct CalendarSimulationResult {
    std::size_t entries = 0;
    double segmentTreeLazyMs = 0.0;
    double fenwickPointUpdateMs = 0.0;
    double avlUpdateQueryMs = 0.0;
    double segmentTreeSum = 0.0;
    double fenwickSum = 0.0;
    double avlSum = 0.0;
    std::size_t memoryBytes = 0;
};

class RangeAnalytics {
public:
    static std::vector<RangeMetric> comparePriceRange(
        const std::vector<Listing>& listings,
        double minPrice,
        double maxPrice
    );

    static CalendarSimulationResult simulateCalendarUpdates(const std::vector<CalendarEntry>& entries);
};

