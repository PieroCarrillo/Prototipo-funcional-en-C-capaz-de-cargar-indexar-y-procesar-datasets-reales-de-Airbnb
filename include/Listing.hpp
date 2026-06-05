#pragma once

#include <string>

struct Listing {
    long long id = 0;
    long long hostId = 0;
    std::string name;
    std::string hostName;
    std::string neighbourhood;
    std::string roomType;
    double price = 0.0;
    int minimumNights = 0;
    int numberOfReviews = 0;
    double reviewsPerMonth = 0.0;
    int availability365 = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    std::string sourceFile;

    double score() const {
        const double reviewWeight = static_cast<double>(numberOfReviews) * 0.70;
        const double availabilityWeight = static_cast<double>(availability365) * 0.02;
        const double pricePenalty = price * 0.03;
        return reviewWeight + availabilityWeight - pricePenalty;
    }
};

