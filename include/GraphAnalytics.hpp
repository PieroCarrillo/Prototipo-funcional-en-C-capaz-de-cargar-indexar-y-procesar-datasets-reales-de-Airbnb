#pragma once

#include "Listing.hpp"

#include <string>
#include <vector>

struct GraphEdge {
    int from = 0;
    int to = 0;
    double weight = 0.0;
};

struct WeightedGraph {
    std::vector<std::string> labels;
    std::vector<std::vector<GraphEdge>> adjacency;
    std::vector<GraphEdge> edges;
};

struct ShortestPathSummary {
    std::string algorithm;
    double elapsedMs = 0.0;
    std::size_t reachableNodes = 0;
    double totalDistance = 0.0;
    std::size_t memoryBytes = 0;
};

struct MstSummary {
    std::string algorithm;
    double elapsedMs = 0.0;
    double totalCost = 0.0;
    std::size_t edgesUsed = 0;
    std::size_t memoryBytes = 0;
};

struct TraversalSummary {
    double dfsElapsedMs = 0.0;
    std::size_t dfsVisited = 0;
    double nodesPerSecond = 0.0;
    double tarjanElapsedMs = 0.0;
    std::vector<std::string> articulationPoints;
};

class GraphAnalytics {
public:
    static WeightedGraph buildListingProximityGraph(const std::vector<Listing>& listings, std::size_t nearestNeighbors);
    static WeightedGraph buildNeighbourhoodGraph(const std::vector<Listing>& listings);

    static ShortestPathSummary runDijkstra(const WeightedGraph& graph, int source);
    static ShortestPathSummary runBellmanFord(const WeightedGraph& graph, int source);
    static ShortestPathSummary runFloydWarshall(const WeightedGraph& graph);

    static MstSummary runKruskal(const WeightedGraph& graph);
    static MstSummary runPrim(const WeightedGraph& graph);
    static MstSummary runBoruvka(const WeightedGraph& graph);
    static TraversalSummary runDfsAndTarjan(const WeightedGraph& graph);
};

