#pragma once

#include <string>

/**
 * @brief Representa un alojamiento obtenido desde un dataset de Airbnb.
 *
 * La estructura concentra los campos utilizados por los modulos de carga,
 * indexacion, busqueda textual, rangos y analisis de grafos.
 */
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

    /**
     * @brief Calcula un puntaje preliminar para ordenar alojamientos.
     *
     * El puntaje favorece la cantidad de resenas y la disponibilidad, mientras
     * aplica una penalizacion moderada al precio. No pretende reemplazar una
     * recomendacion comercial definitiva; sirve para demostrar el ranking.
     */
    double score() const {
        const double reviewWeight = static_cast<double>(numberOfReviews) * 0.70;
        const double availabilityWeight = static_cast<double>(availability365) * 0.02;
        const double pricePenalty = price * 0.03;
        return reviewWeight + availabilityWeight - pricePenalty;
    }
};
