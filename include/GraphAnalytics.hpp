#pragma once

#include "Listing.hpp"

#include <string>
#include <vector>

/// Arista ponderada utilizada para representar una relacion geografica.
struct GraphEdge {
    int from = 0;
    int to = 0;
    double weight = 0.0;
};

/**
 * @brief Grafo ponderado almacenado mediante lista de adyacencia.
 *
 * `labels` identifica los nodos, `adjacency` facilita los recorridos y
 * `edges` permite ejecutar algoritmos que procesan todas las aristas.
 */
struct WeightedGraph {
    std::vector<std::string> labels;
    std::vector<std::vector<GraphEdge>> adjacency;
    std::vector<GraphEdge> edges;
};

/// Resultado comun de los algoritmos de caminos minimos del modulo M4.
struct ShortestPathSummary {
    std::string algorithm;
    double elapsedMs = 0.0;
    std::size_t reachableNodes = 0;
    double totalDistance = 0.0;
    std::size_t memoryBytes = 0;
};

/// Resultado comun de los algoritmos de arbol de expansion minima.
struct MstSummary {
    std::string algorithm;
    double elapsedMs = 0.0;
    double totalCost = 0.0;
    std::size_t edgesUsed = 0;
    std::size_t memoryBytes = 0;
};

/// Metricas obtenidas por DFS y Tarjan sobre el grafo de barrios.
struct TraversalSummary {
    double dfsElapsedMs = 0.0;
    std::size_t dfsVisited = 0;
    double nodesPerSecond = 0.0;
    double tarjanElapsedMs = 0.0;
    std::vector<std::string> articulationPoints;
};

/**
 * @brief Implementa el modulo M4 de busqueda y analisis en grafos.
 *
 * Los alojamientos se conectan por proximidad geografica y los barrios se
 * representan mediante sus centroides. El modulo compara caminos minimos,
 * arboles de expansion minima, recorridos y puntos de articulacion.
 */
class GraphAnalytics {
public:
    /**
     * Construye un grafo de alojamientos conectado por vecinos cercanos.
     * Los pesos corresponden a distancia Haversine expresada en kilometros.
     */
    static WeightedGraph buildListingProximityGraph(const std::vector<Listing>& listings, std::size_t nearestNeighbors);

    /// Construye un grafo completo de barrios a partir de centroides promedio.
    static WeightedGraph buildNeighbourhoodGraph(const std::vector<Listing>& listings);

    /// Ejecuta Dijkstra desde un nodo origen sobre pesos no negativos.
    static ShortestPathSummary runDijkstra(const WeightedGraph& graph, int source);

    /// Ejecuta Bellman-Ford mediante relajacion repetida de aristas.
    static ShortestPathSummary runBellmanFord(const WeightedGraph& graph, int source);

    /// Ejecuta Floyd-Warshall para obtener caminos entre todos los pares.
    static ShortestPathSummary runFloydWarshall(const WeightedGraph& graph);

    /// Calcula el MST ordenando aristas y uniendo componentes disjuntos.
    static MstSummary runKruskal(const WeightedGraph& graph);

    /// Calcula el MST expandiendo el arbol desde una cola de prioridad.
    static MstSummary runPrim(const WeightedGraph& graph);

    /// Calcula el MST eligiendo la arista minima de cada componente.
    static MstSummary runBoruvka(const WeightedGraph& graph);

    /// Ejecuta DFS y Tarjan para recorrido y puntos de articulacion.
    static TraversalSummary runDfsAndTarjan(const WeightedGraph& graph);
};
