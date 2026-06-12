# Validacion con el dataset real de Paris

## Dataset recibido

El archivo `Paris.zip` entregado para el proyecto contiene la coleccion de
Inside Airbnb correspondiente a Paris:

| Archivo | Contenido |
| --- | --- |
| `listings.csv` | 82,467 alojamientos resumidos |
| `listings.csv.gz` | Detalle ampliado de los alojamientos |
| `reviews.csv` | 2,281,438 resenas resumidas |
| `reviews.csv.gz` | Resenas detalladas con comentarios |
| `calendar.csv.gz` | 30,100,525 registros diarios |
| `neighbourhoods.csv` | 20 barrios |
| `neighbourhoods.geojson` | 20 poligonos geograficos |

El archivo ZIP no se almacena en GitHub debido a su tamano. El script
`import_paris.bat` extrae los archivos resumidos en `data/real/Paris`, carpeta
excluida mediante `.gitignore`.

## Ajustes realizados

- El lector CSV admite campos con comas, comillas escapadas y saltos de linea.
- Los precios ausentes se representan como `N/D`, no como precio cero.
- Los indices de precio solo incluyen registros que publican un precio.
- El grafo usa todos los barrios, pero limita Floyd-Warshall a una muestra
  configurable de listings para controlar su complejidad O(V^3).
- Se genera una muestra de 100,000 filas disponibles del calendario.
- Como el calendario de Paris no publica precios, M5 usa disponibilidad
  binaria: disponible = 1 y no disponible = 0.

## Resultados de carga

| Metrica | Resultado |
| --- | ---: |
| Listings leidos | 82,467 |
| Listings cargados | 82,467 |
| Filas omitidas | 0 |
| Barrios detectados | 20 |
| Calendarios cargados | 100,000 |
| Memoria estimada de indices | 45,116,134 bytes |

## Resultados funcionales

- La busqueda exacta encontro el listing `43675393`.
- La busqueda textual por `Paris` devolvio alojamientos relevantes.
- El rango de precio 40-120 produjo 18,082 coincidencias.
- El ordenamiento ignora precios ausentes y comienza en el menor precio
  publicado.

## Modulo M4

El grafo de proximidad se valido con una muestra de 100 listings:

| Algoritmo | Resultado |
| --- | --- |
| Dijkstra | 100 nodos alcanzables |
| Bellman-Ford | 100 nodos alcanzables |
| Floyd-Warshall | Mismas distancias desde el origen |

El grafo de barrios contiene 20 nodos y 190 aristas. Kruskal, Prim y Boruvka
obtuvieron un MST de 19 aristas y costo aproximado de 29.006 km.

## Modulo M5

Las tres estrategias devolvieron 18,082 coincidencias para el rango 40-120:

| Estructura | Coincidencias |
| --- | ---: |
| Segment Tree | 18,082 |
| Fenwick Tree | 18,082 |
| Busqueda binaria | 18,082 |

Para 100,000 registros de calendario, Segment Tree lazy, Fenwick puntual y AVL
obtuvieron el mismo valor acumulado final: 100,002.

## Ejecucion

```cmd
.\import_paris.bat
.\run_paris.bat
```
