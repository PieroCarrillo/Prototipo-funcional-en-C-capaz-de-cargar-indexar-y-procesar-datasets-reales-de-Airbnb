# Resultados preliminares de busqueda y ordenamiento

Los siguientes resultados corresponden a la coleccion piloto ubicada en `data/pilot/`, formada por 12 alojamientos distribuidos en Lima y Cusco.

## Comando base

```bash
./build/airbnb_indexer --data data/pilot --query playa --id 1001 --min-price 40 --max-price 120 --top 5
```

## Recorrido de directorios

Archivos esperados:

| Tipo | Archivo |
| --- | --- |
| listings | `data/pilot/cusco/listings.csv` |
| listings | `data/pilot/lima/listings.csv` |
| reviews | `data/pilot/lima/reviews.csv` |
| calendar | `data/pilot/lima/calendar.csv` |

Resumen esperado:

| Metrica | Valor |
| --- | ---: |
| Archivos `listings.csv` | 2 |
| Filas de listings leidas | 12 |
| Listings cargados | 12 |
| Filas omitidas | 0 |

## Busqueda exacta

Entrada:

```bash
--id 1001
```

Resultado esperado:

| ID | Nombre | Barrio | Precio |
| ---: | --- | --- | ---: |
| 1001 | Departamento luminoso cerca a la playa | Miraflores | 85 |

## Busqueda parcial

Entrada:

```bash
--query playa
```

Resultado esperado:

| ID | Nombre | Barrio | Precio |
| ---: | --- | --- | ---: |
| 1001 | Departamento luminoso cerca a la playa | Miraflores | 85 |

## Filtro por rango de precio

Entrada:

```bash
--min-price 40 --max-price 120 --top 5
```

Primeros resultados esperados, ordenados por precio:

| ID | Nombre | Precio |
| ---: | --- | ---: |
| 1002 | Habitacion privada en Barranco cultural | 42 |
| 2006 | Estudio compacto para teletrabajo | 58 |
| 2003 | Casa tranquila en San Blas | 76 |
| 1001 | Departamento luminoso cerca a la playa | 85 |
| 2002 | Departamento colonial cerca a la plaza | 95 |

## Ordenamiento por precio ascendente

Primeros 5:

| ID | Nombre | Precio |
| ---: | --- | ---: |
| 2001 | Hostal centrico para mochileros | 24 |
| 1006 | Habitacion economica cerca al Metropolitano | 29 |
| 2004 | Habitacion con vista a montanas | 35 |
| 1004 | Mini estudio para viajes cortos | 38 |
| 1002 | Habitacion privada en Barranco cultural | 42 |

## Ranking por score

El score preliminar se calcula como:

```text
score = number_of_reviews * 0.70 + availability_365 * 0.02 - price * 0.03
```

Primeros 5:

| ID | Nombre | Score aproximado |
| ---: | --- | ---: |
| 2001 | Hostal centrico para mochileros | 152.68 |
| 2002 | Departamento colonial cerca a la plaza | 122.25 |
| 1001 | Departamento luminoso cerca a la playa | 102.45 |
| 2003 | Casa tranquila en San Blas | 85.12 |
| 1005 | Casa familiar con terraza y cocina amplia | 70.50 |

## Interpretacion

La coleccion piloto confirma que el prototipo cubre operaciones esenciales del segundo avance: localiza archivos en subdirectorios, evita busqueda secuencial para consultas por ID, utiliza indice invertido para texto, filtra por rango de precio con estructura ordenada y genera un ranking inicial mediante cola de prioridad.

