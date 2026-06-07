#include "GraphAnalytics.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <set>
#include <unordered_map>

namespace {
using Clock = std::chrono::steady_clock;
constexpr double EarthRadiusKm = 6371.0;
constexpr double Inf = std::numeric_limits<double>::infinity();

double elapsedMs(const Clock::time_point& start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

double toRadians(double degrees) {
    return degrees * 3.14159265358979323846 / 180.0;
}

double haversineKm(double lat1, double lon1, double lat2, double lon2) {
    const double dLat = toRadians(lat2 - lat1);
    const double dLon = toRadians(lon2 - lon1);
    const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
        std::cos(toRadians(lat1)) * std::cos(toRadians(lat2)) *
        std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EarthRadiusKm * c;
}

void addUndirectedEdge(WeightedGraph& graph, int from, int to, double weight) {
    GraphEdge edge{from, to, weight};
    graph.edges.push_back(edge);
    graph.adjacency[from].push_back(edge);
    graph.adjacency[to].push_back({to, from, weight});
}

struct DisjointSet {
    std::vector<int> parent;
    std::vector<int> rank;

    explicit DisjointSet(std::size_t size) : parent(size), rank(size, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }

    int find(int value) {
        if (parent[value] != value) {
            parent[value] = find(parent[value]);
        }
        return parent[value];
    }

    bool unite(int left, int right) {
        int rootLeft = find(left);
        int rootRight = find(right);
        if (rootLeft == rootRight) {
            return false;
        }
        if (rank[rootLeft] < rank[rootRight]) {
            std::swap(rootLeft, rootRight);
        }
        parent[rootRight] = rootLeft;
        if (rank[rootLeft] == rank[rootRight]) {
            ++rank[rootLeft];
        }
        return true;
    }
};

std::size_t graphMemoryBytes(const WeightedGraph& graph) {
    std::size_t total = sizeof(graph) + graph.edges.capacity() * sizeof(GraphEdge);
    for (const auto& label : graph.labels) {
        total += label.capacity();
    }
    for (const auto& list : graph.adjacency) {
        total += list.capacity() * sizeof(GraphEdge);
    }
    return total;
}

ShortestPathSummary summarizeDistances(
    const std::string& algorithm,
    const std::vector<double>& distances,
    double ms,
    std::size_t memoryBytes
) {
    ShortestPathSummary summary;
    summary.algorithm = algorithm;
    summary.elapsedMs = ms;
    summary.memoryBytes = memoryBytes + distances.capacity() * sizeof(double);
    for (double distance : distances) {
        if (std::isfinite(distance)) {
            ++summary.reachableNodes;
            summary.totalDistance += distance;
        }
    }
    return summary;
}
}

WeightedGraph GraphAnalytics::buildListingProximityGraph(
    const std::vector<Listing>& listings,
    std::size_t nearestNeighbors
) {
    WeightedGraph graph;
    graph.labels.reserve(listings.size());
    graph.adjacency.resize(listings.size());
    for (const Listing& listing : listings) {
        graph.labels.push_back("#" + std::to_string(listing.id) + " " + listing.neighbourhood);
    }

    std::set<std::pair<int, int>> usedPairs;
    for (std::size_t i = 0; i < listings.size(); ++i) {
        std::vector<std::pair<double, int>> distances;
        for (std::size_t j = 0; j < listings.size(); ++j) {
            if (i == j) {
                continue;
            }
            distances.push_back({
                haversineKm(
                    listings[i].latitude,
                    listings[i].longitude,
                    listings[j].latitude,
                    listings[j].longitude
                ),
                static_cast<int>(j)
            });
        }
        std::sort(distances.begin(), distances.end());

        const std::size_t limit = std::min(nearestNeighbors, distances.size());
        for (std::size_t k = 0; k < limit; ++k) {
            const int from = static_cast<int>(i);
            const int to = distances[k].second;
            const auto key = std::minmax(from, to);
            if (usedPairs.insert(key).second) {
                addUndirectedEdge(graph, from, to, distances[k].first);
            }
        }
    }

    for (std::size_t i = 1; i < listings.size(); ++i) {
        const int from = static_cast<int>(i - 1);
        const int to = static_cast<int>(i);
        const auto key = std::minmax(from, to);
        if (usedPairs.insert(key).second) {
            addUndirectedEdge(
                graph,
                from,
                to,
                haversineKm(
                    listings[from].latitude,
                    listings[from].longitude,
                    listings[to].latitude,
                    listings[to].longitude
                )
            );
        }
    }
    return graph;
}

WeightedGraph GraphAnalytics::buildNeighbourhoodGraph(const std::vector<Listing>& listings) {
    struct Accumulator {
        double lat = 0.0;
        double lon = 0.0;
        int count = 0;
    };

    std::unordered_map<std::string, Accumulator> byNeighbourhood;
    for (const Listing& listing : listings) {
        auto& item = byNeighbourhood[listing.neighbourhood];
        item.lat += listing.latitude;
        item.lon += listing.longitude;
        ++item.count;
    }

    WeightedGraph graph;
    std::vector<std::pair<std::string, Accumulator>> centroids(byNeighbourhood.begin(), byNeighbourhood.end());
    std::sort(centroids.begin(), centroids.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });

    graph.labels.reserve(centroids.size());
    graph.adjacency.resize(centroids.size());
    for (const auto& item : centroids) {
        graph.labels.push_back(item.first);
    }

    for (std::size_t i = 0; i < centroids.size(); ++i) {
        for (std::size_t j = i + 1; j < centroids.size(); ++j) {
            const auto& a = centroids[i].second;
            const auto& b = centroids[j].second;
            const double distance = haversineKm(
                a.lat / a.count,
                a.lon / a.count,
                b.lat / b.count,
                b.lon / b.count
            );
            addUndirectedEdge(graph, static_cast<int>(i), static_cast<int>(j), distance);
        }
    }

    return graph;
}

ShortestPathSummary GraphAnalytics::runDijkstra(const WeightedGraph& graph, int source) {
    const auto start = Clock::now();
    std::vector<double> distances(graph.labels.size(), Inf);
    using Item = std::pair<double, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;

    distances[source] = 0.0;
    queue.push({0.0, source});
    while (!queue.empty()) {
        const auto [distance, node] = queue.top();
        queue.pop();
        if (distance > distances[node]) {
            continue;
        }
        for (const GraphEdge& edge : graph.adjacency[node]) {
            const double candidate = distance + edge.weight;
            if (candidate < distances[edge.to]) {
                distances[edge.to] = candidate;
                queue.push({candidate, edge.to});
            }
        }
    }

    return summarizeDistances("Dijkstra", distances, elapsedMs(start), graphMemoryBytes(graph));
}

ShortestPathSummary GraphAnalytics::runBellmanFord(const WeightedGraph& graph, int source) {
    const auto start = Clock::now();
    std::vector<double> distances(graph.labels.size(), Inf);
    distances[source] = 0.0;

    for (std::size_t i = 1; i < graph.labels.size(); ++i) {
        bool changed = false;
        for (const GraphEdge& edge : graph.edges) {
            if (std::isfinite(distances[edge.from]) && distances[edge.from] + edge.weight < distances[edge.to]) {
                distances[edge.to] = distances[edge.from] + edge.weight;
                changed = true;
            }
            if (std::isfinite(distances[edge.to]) && distances[edge.to] + edge.weight < distances[edge.from]) {
                distances[edge.from] = distances[edge.to] + edge.weight;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    return summarizeDistances("Bellman-Ford", distances, elapsedMs(start), graphMemoryBytes(graph));
}

ShortestPathSummary GraphAnalytics::runFloydWarshall(const WeightedGraph& graph) {
    const auto start = Clock::now();
    const std::size_t n = graph.labels.size();
    std::vector<std::vector<double>> distances(n, std::vector<double>(n, Inf));
    for (std::size_t i = 0; i < n; ++i) {
        distances[i][i] = 0.0;
    }
    for (const GraphEdge& edge : graph.edges) {
        distances[edge.from][edge.to] = std::min(distances[edge.from][edge.to], edge.weight);
        distances[edge.to][edge.from] = std::min(distances[edge.to][edge.from], edge.weight);
    }

    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                if (distances[i][k] + distances[k][j] < distances[i][j]) {
                    distances[i][j] = distances[i][k] + distances[k][j];
                }
            }
        }
    }

    std::vector<double> fromFirst = n == 0 ? std::vector<double>{} : distances.front();
    std::size_t memory = graphMemoryBytes(graph) + n * n * sizeof(double);
    return summarizeDistances("Floyd-Warshall", fromFirst, elapsedMs(start), memory);
}

MstSummary GraphAnalytics::runKruskal(const WeightedGraph& graph) {
    const auto start = Clock::now();
    std::vector<GraphEdge> edges = graph.edges;
    std::sort(edges.begin(), edges.end(), [](const GraphEdge& left, const GraphEdge& right) {
        return left.weight < right.weight;
    });

    DisjointSet set(graph.labels.size());
    MstSummary summary;
    summary.algorithm = "Kruskal";
    for (const GraphEdge& edge : edges) {
        if (set.unite(edge.from, edge.to)) {
            summary.totalCost += edge.weight;
            ++summary.edgesUsed;
        }
    }
    summary.elapsedMs = elapsedMs(start);
    summary.memoryBytes = graphMemoryBytes(graph) + edges.capacity() * sizeof(GraphEdge);
    return summary;
}

MstSummary GraphAnalytics::runPrim(const WeightedGraph& graph) {
    const auto start = Clock::now();
    MstSummary summary;
    summary.algorithm = "Prim";
    if (graph.labels.empty()) {
        return summary;
    }

    using Item = std::pair<double, int>;
    std::vector<bool> used(graph.labels.size(), false);
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;
    queue.push({0.0, 0});

    while (!queue.empty()) {
        const auto [weight, node] = queue.top();
        queue.pop();
        if (used[node]) {
            continue;
        }
        used[node] = true;
        summary.totalCost += weight;
        if (weight > 0.0) {
            ++summary.edgesUsed;
        }
        for (const GraphEdge& edge : graph.adjacency[node]) {
            if (!used[edge.to]) {
                queue.push({edge.weight, edge.to});
            }
        }
    }

    summary.elapsedMs = elapsedMs(start);
    summary.memoryBytes = graphMemoryBytes(graph) + used.capacity() * sizeof(bool);
    return summary;
}

MstSummary GraphAnalytics::runBoruvka(const WeightedGraph& graph) {
    const auto start = Clock::now();
    MstSummary summary;
    summary.algorithm = "Boruvka";
    const int n = static_cast<int>(graph.labels.size());
    if (n == 0) {
        return summary;
    }

    DisjointSet set(graph.labels.size());
    int components = n;
    std::vector<int> cheapest(n, -1);
    while (components > 1) {
        std::fill(cheapest.begin(), cheapest.end(), -1);
        for (std::size_t i = 0; i < graph.edges.size(); ++i) {
            const GraphEdge& edge = graph.edges[i];
            int left = set.find(edge.from);
            int right = set.find(edge.to);
            if (left == right) {
                continue;
            }
            if (cheapest[left] == -1 || graph.edges[cheapest[left]].weight > edge.weight) {
                cheapest[left] = static_cast<int>(i);
            }
            if (cheapest[right] == -1 || graph.edges[cheapest[right]].weight > edge.weight) {
                cheapest[right] = static_cast<int>(i);
            }
        }

        bool changed = false;
        for (int edgeIndex : cheapest) {
            if (edgeIndex == -1) {
                continue;
            }
            const GraphEdge& edge = graph.edges[edgeIndex];
            if (set.unite(edge.from, edge.to)) {
                summary.totalCost += edge.weight;
                ++summary.edgesUsed;
                --components;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }

    summary.elapsedMs = elapsedMs(start);
    summary.memoryBytes = graphMemoryBytes(graph) + cheapest.capacity() * sizeof(int);
    return summary;
}

TraversalSummary GraphAnalytics::runDfsAndTarjan(const WeightedGraph& graph) {
    TraversalSummary summary;
    const std::size_t n = graph.labels.size();
    std::vector<bool> visited(n, false);

    const auto dfsStart = Clock::now();
    std::vector<int> stack;
    if (n > 0) {
        stack.push_back(0);
        visited[0] = true;
    }
    while (!stack.empty()) {
        int node = stack.back();
        stack.pop_back();
        ++summary.dfsVisited;
        for (const GraphEdge& edge : graph.adjacency[node]) {
            if (!visited[edge.to]) {
                visited[edge.to] = true;
                stack.push_back(edge.to);
            }
        }
    }
    summary.dfsElapsedMs = elapsedMs(dfsStart);
    if (summary.dfsElapsedMs > 0.0) {
        summary.nodesPerSecond = summary.dfsVisited / (summary.dfsElapsedMs / 1000.0);
    }

    const auto tarjanStart = Clock::now();
    std::vector<int> disc(n, -1);
    std::vector<int> low(n, -1);
    std::vector<int> parent(n, -1);
    std::vector<bool> articulation(n, false);
    int timer = 0;

    auto dfs = [&](auto&& self, int node) -> void {
        disc[node] = low[node] = timer++;
        int children = 0;
        for (const GraphEdge& edge : graph.adjacency[node]) {
            int next = edge.to;
            if (disc[next] == -1) {
                ++children;
                parent[next] = node;
                self(self, next);
                low[node] = std::min(low[node], low[next]);
                if (parent[node] == -1 && children > 1) {
                    articulation[node] = true;
                }
                if (parent[node] != -1 && low[next] >= disc[node]) {
                    articulation[node] = true;
                }
            } else if (next != parent[node]) {
                low[node] = std::min(low[node], disc[next]);
            }
        }
    };

    for (std::size_t i = 0; i < n; ++i) {
        if (disc[i] == -1) {
            dfs(dfs, static_cast<int>(i));
        }
    }

    summary.tarjanElapsedMs = elapsedMs(tarjanStart);
    for (std::size_t i = 0; i < n; ++i) {
        if (articulation[i]) {
            summary.articulationPoints.push_back(graph.labels[i]);
        }
    }
    return summary;
}
