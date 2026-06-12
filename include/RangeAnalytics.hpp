#pragma once

#include "DataLoader.hpp"
#include "Listing.hpp"

#include <cstddef>
#include <string>
#include <vector>

/// Metricas comparables de una consulta por rango de precio.
struct RangeMetric {
    std::string algorithm;
    std::size_t matches = 0;
    double elapsedMs = 0.0;
    double throughputQueriesPerSecond = 0.0;
    std::size_t memoryBytes = 0;
};

/// Resultado de simular actualizaciones sobre registros de calendario.
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

/**
 * @brief Implementa el modulo M5 de consultas por rangos.
 *
 * Compara estructuras estaticas para rangos de precio y estructuras
 * dinamicas para actualizaciones de los precios del calendario.
 */
class RangeAnalytics {
public:
    /**
     * Compara Segment Tree, Fenwick Tree y busqueda binaria.
     * Las tres estrategias deben devolver la misma cantidad de coincidencias.
     */
    static std::vector<RangeMetric> comparePriceRange(
        const std::vector<Listing>& listings,
        double minPrice,
        double maxPrice
    );

    /**
     * Simula una actualizacion usando Segment Tree con lazy propagation,
     * Fenwick Tree con actualizacion puntual y arbol AVL. Se usa el precio
     * cuando esta disponible; de lo contrario se representa disponibilidad
     * como 1 y no disponibilidad como 0.
     */
    static CalendarSimulationResult simulateCalendarUpdates(const std::vector<CalendarEntry>& entries);
};
