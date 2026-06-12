#include "AirbnbIndex.hpp"

#include <algorithm>
#include <cctype>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>

// La coleccion base conserva cada Listing una sola vez. Los indices almacenan
// posiciones para evitar duplicar registros completos en memoria.
void AirbnbIndex::add(Listing listing) {
    listings_.push_back(std::move(listing));
}

// Reconstruye todos los indices. Esta operacion se realiza despues de terminar
// la carga para que las consultas posteriores sean rapidas.
void AirbnbIndex::build() {
    byId_.clear();
    textIndex_.clear();
    byNeighbourhood_.clear();
    byPrice_.clear();

    for (std::size_t index = 0; index < listings_.size(); ++index) {
        const Listing& listing = listings_[index];
        byId_[listing.id] = index;
        byPrice_.insert({listing.price, index});
        byNeighbourhood_[normalize(listing.neighbourhood)].push_back(index);

        std::unordered_set<std::string> uniqueTokens;
        const std::string searchable = listing.name + " " + listing.hostName + " " +
            listing.neighbourhood + " " + listing.roomType;
        for (const std::string& token : tokenize(searchable)) {
            if (!token.empty()) {
                uniqueTokens.insert(token);
            }
        }

        for (const std::string& token : uniqueTokens) {
            textIndex_[token].push_back(index);
        }
    }
}

std::size_t AirbnbIndex::size() const {
    return listings_.size();
}

const std::vector<Listing>& AirbnbIndex::listings() const {
    return listings_;
}

const Listing* AirbnbIndex::findById(long long id) const {
    auto it = byId_.find(id);
    if (it == byId_.end()) {
        return nullptr;
    }
    return &listings_[it->second];
}

std::vector<const Listing*> AirbnbIndex::searchText(const std::string& query, std::size_t limit) const {
    std::vector<std::string> queryTokens = tokenize(query);
    if (queryTokens.empty()) {
        return {};
    }

    // La frecuencia funciona como una relevancia simple: un registro que
    // coincide con mas tokens aparece primero en el resultado.
    std::unordered_map<std::size_t, int> frequency;
    for (const std::string& token : queryTokens) {
        auto exactIt = textIndex_.find(token);
        if (exactIt != textIndex_.end()) {
            for (std::size_t position : exactIt->second) {
                ++frequency[position];
            }
            continue;
        }

        for (const auto& item : textIndex_) {
            if (item.first.find(token) != std::string::npos) {
                for (std::size_t position : item.second) {
                    ++frequency[position];
                }
            }
        }
    }

    std::vector<std::pair<std::size_t, int>> ranked(frequency.begin(), frequency.end());
    std::sort(ranked.begin(), ranked.end(), [this](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return listings_[left.first].score() > listings_[right.first].score();
    });

    std::vector<const Listing*> result;
    for (const auto& item : ranked) {
        if (result.size() >= limit) {
            break;
        }
        result.push_back(&listings_[item.first]);
    }
    return result;
}

std::vector<const Listing*> AirbnbIndex::filterByPrice(double minPrice, double maxPrice, std::size_t limit) const {
    // multimap mantiene las claves ordenadas y permite iniciar directamente en
    // el limite inferior, sin recorrer precios menores al rango solicitado.
    std::vector<const Listing*> result;
    for (auto it = byPrice_.lower_bound(minPrice); it != byPrice_.end() && it->first <= maxPrice; ++it) {
        result.push_back(&listings_[it->second]);
        if (result.size() >= limit) {
            break;
        }
    }
    return result;
}

std::vector<const Listing*> AirbnbIndex::sortByPrice(bool ascending, std::size_t limit) const {
    std::vector<const Listing*> result;
    if (ascending) {
        for (const auto& item : byPrice_) {
            result.push_back(&listings_[item.second]);
            if (result.size() >= limit) {
                break;
            }
        }
    } else {
        for (auto it = byPrice_.rbegin(); it != byPrice_.rend(); ++it) {
            result.push_back(&listings_[it->second]);
            if (result.size() >= limit) {
                break;
            }
        }
    }
    return result;
}

std::vector<const Listing*> AirbnbIndex::sortByReviews(std::size_t limit) const {
    std::vector<std::size_t> positions(listings_.size());
    for (std::size_t i = 0; i < listings_.size(); ++i) {
        positions[i] = i;
    }

    std::sort(positions.begin(), positions.end(), [this](std::size_t left, std::size_t right) {
        if (listings_[left].numberOfReviews != listings_[right].numberOfReviews) {
            return listings_[left].numberOfReviews > listings_[right].numberOfReviews;
        }
        return listings_[left].price < listings_[right].price;
    });
    return collectByPositions(positions, limit);
}

std::vector<const Listing*> AirbnbIndex::topRanked(std::size_t limit) const {
    // La cola de prioridad conserva primero el alojamiento con mayor score.
    auto compare = [this](std::size_t left, std::size_t right) {
        return listings_[left].score() < listings_[right].score();
    };

    std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(compare)> queue(compare);
    for (std::size_t i = 0; i < listings_.size(); ++i) {
        queue.push(i);
    }

    std::vector<const Listing*> result;
    while (!queue.empty() && result.size() < limit) {
        result.push_back(&listings_[queue.top()]);
        queue.pop();
    }
    return result;
}

std::size_t AirbnbIndex::estimatedMemoryBytes() const {
    // La STL no expone el consumo exacto del asignador. Esta estimacion usa
    // tamanos y capacidades para producir una metrica comparable.
    std::size_t total = sizeof(*this) + listings_.capacity() * sizeof(Listing);
    total += byId_.size() * (sizeof(long long) + sizeof(std::size_t));
    total += byPrice_.size() * (sizeof(double) + sizeof(std::size_t));

    for (const auto& item : textIndex_) {
        total += item.first.capacity();
        total += item.second.capacity() * sizeof(std::size_t);
    }

    for (const auto& item : byNeighbourhood_) {
        total += item.first.capacity();
        total += item.second.capacity() * sizeof(std::size_t);
    }
    return total;
}

std::vector<std::string> AirbnbIndex::tokenize(const std::string& text) {
    const std::string normalized = normalize(text);
    std::vector<std::string> tokens;
    std::string current;

    for (char ch : normalized) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            current.push_back(ch);
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::string AirbnbIndex::normalize(const std::string& text) {
    std::string value;
    value.reserve(text.size());
    for (unsigned char ch : text) {
        value.push_back(static_cast<char>(std::tolower(ch)));
    }
    return value;
}

std::vector<const Listing*> AirbnbIndex::collectByPositions(
    const std::vector<std::size_t>& positions,
    std::size_t limit
) const {
    std::vector<const Listing*> result;
    for (std::size_t position : positions) {
        if (result.size() >= limit) {
            break;
        }
        result.push_back(&listings_[position]);
    }
    return result;
}
