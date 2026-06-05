#pragma once

#include "Listing.hpp"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class AirbnbIndex {
public:
    void add(Listing listing);
    void build();

    std::size_t size() const;
    const std::vector<Listing>& listings() const;

    const Listing* findById(long long id) const;
    std::vector<const Listing*> searchText(const std::string& query, std::size_t limit) const;
    std::vector<const Listing*> filterByPrice(double minPrice, double maxPrice, std::size_t limit) const;
    std::vector<const Listing*> sortByPrice(bool ascending, std::size_t limit) const;
    std::vector<const Listing*> sortByReviews(std::size_t limit) const;
    std::vector<const Listing*> topRanked(std::size_t limit) const;
    std::size_t estimatedMemoryBytes() const;

private:
    std::vector<Listing> listings_;
    std::unordered_map<long long, std::size_t> byId_;
    std::unordered_map<std::string, std::vector<std::size_t>> textIndex_;
    std::unordered_map<std::string, std::vector<std::size_t>> byNeighbourhood_;
    std::multimap<double, std::size_t> byPrice_;

    static std::vector<std::string> tokenize(const std::string& text);
    static std::string normalize(const std::string& text);
    std::vector<const Listing*> collectByPositions(const std::vector<std::size_t>& positions, std::size_t limit) const;
};

