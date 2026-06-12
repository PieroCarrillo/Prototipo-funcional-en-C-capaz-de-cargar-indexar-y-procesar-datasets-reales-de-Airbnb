#pragma once

#include "Listing.hpp"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Motor de indexacion y consulta en memoria para alojamientos.
 *
 * Mantiene una coleccion base en vector y varios indices auxiliares para
 * resolver busquedas exactas, textuales, ordenamientos, rankings y rangos.
 */
class AirbnbIndex {
public:
    /// Incorpora un alojamiento a la coleccion base.
    void add(Listing listing);

    /// Reconstruye todos los indices a partir de la coleccion cargada.
    void build();

    /// Devuelve la cantidad total de alojamientos.
    std::size_t size() const;

    /// Expone la coleccion para los modulos analiticos de grafos y rangos.
    const std::vector<Listing>& listings() const;

    /// Busca un alojamiento por ID en tiempo promedio O(1).
    const Listing* findById(long long id) const;

    /// Recupera coincidencias textuales exactas o parciales mediante tokens.
    std::vector<const Listing*> searchText(const std::string& query, std::size_t limit) const;

    /// Recupera alojamientos cuyo precio pertenece al intervalo solicitado.
    std::vector<const Listing*> filterByPrice(double minPrice, double maxPrice, std::size_t limit) const;

    /// Ordena los alojamientos por precio en sentido ascendente o descendente.
    std::vector<const Listing*> sortByPrice(bool ascending, std::size_t limit) const;

    /// Ordena los alojamientos de acuerdo con su cantidad de resenas.
    std::vector<const Listing*> sortByReviews(std::size_t limit) const;

    /// Obtiene los alojamientos con mayor score mediante una cola de prioridad.
    std::vector<const Listing*> topRanked(std::size_t limit) const;

    /// Estima la memoria ocupada por la coleccion y sus indices auxiliares.
    std::size_t estimatedMemoryBytes() const;

private:
    std::vector<Listing> listings_;
    std::unordered_map<long long, std::size_t> byId_;
    std::unordered_map<std::string, std::vector<std::size_t>> textIndex_;
    std::unordered_map<std::string, std::vector<std::size_t>> byNeighbourhood_;
    std::multimap<double, std::size_t> byPrice_;

    /// Separa un texto normalizado en palabras indexables.
    static std::vector<std::string> tokenize(const std::string& text);

    /// Convierte el texto a minusculas para reducir diferencias de busqueda.
    static std::string normalize(const std::string& text);

    /// Convierte posiciones internas en referencias de solo lectura.
    std::vector<const Listing*> collectByPositions(const std::vector<std::size_t>& positions, std::size_t limit) const;
};
