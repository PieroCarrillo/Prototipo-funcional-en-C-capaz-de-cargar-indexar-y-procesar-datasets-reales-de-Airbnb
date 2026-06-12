# Segundo avance: prototipo funcional en C++ para datasets Airbnb

Este repositorio contiene el segundo avance del trabajo final: un prototipo en C++17 capaz de recorrer directorios, cargar archivos CSV tipo Airbnb, construir indices en memoria y ejecutar consultas de busqueda, filtrado, ordenamiento y medicion de rendimiento.

## Criterios cubiertos

| Criterio del segundo avance | Evidencia en el repositorio |
| --- | --- |
| Recorrido de directorios en C++ | `src/DirectoryScanner.cpp` usa `std::filesystem::recursive_directory_iterator` |
| Construccion del indice en memoria | `src/AirbnbIndex.cpp` mantiene `unordered_map`, indice invertido y `multimap` |
| Busqueda exacta y parcial | Busqueda por ID y busqueda textual por tokens o coincidencias parciales |
| Ordenamiento y filtrado | Filtro por rango de precio y ordenamiento por precio, resenas y ranking |
| Metricas de rendimiento | `src/main.cpp` mide carga, indexacion, busqueda y ordenamiento con `chrono` |
| Simulacion con colecciones piloto | `data/pilot/` incluye carpetas por ciudad y archivos CSV de prueba |
| M4 - Busqueda en grafos | `src/GraphAnalytics.cpp` implementa Dijkstra, Bellman-Ford, Floyd-Warshall, Kruskal, Prim, Boruvka, DFS y Tarjan |
| M5 - Busqueda por rangos | `src/RangeAnalytics.cpp` implementa Segment Tree, Fenwick Tree, busqueda binaria, Segment Tree lazy, Fenwick puntual y AVL |

## Estructura

```text
.
|-- CMakeLists.txt
|-- README.md
|-- data/pilot/
|-- docs/AVANCE_2_GRUPO_03.md
|-- include/
|-- src/
`-- tests/casos_prueba.md
```

## Documentacion del codigo

Las clases, estructuras y metodos publicos incluyen comentarios formales con
estilo Doxygen (`@brief`). Visual Studio Code puede mostrar estas descripciones
al posicionar el cursor sobre los simbolos. Las implementaciones tambien
incluyen comentarios de bloque para explicar las etapas, estructuras y
decisiones algoritmicas principales.

## Compilacion

Con CMake:

```bash
cmake -S . -B build
cmake --build build
```

Con g++:

```bash
g++ -std=c++17 -O2 -Iinclude src/*.cpp -o build/airbnb_indexer
```

En Windows, el ejecutable puede quedar como `build/airbnb_indexer.exe`.

## Ejecucion rapida

En Windows, desde VS Code, PowerShell o CMD:

```cmd
.\run.bat
```

Ese archivo compila y ejecuta el prototipo usando `C:\msys64\mingw64\bin\g++.exe`.

En VS Code tambien puedes presionar `Ctrl+Shift+B` y elegir `Compilar y ejecutar Airbnb`.

Tambien puedes compilar manualmente:

```bash
./build/airbnb_indexer --data data/pilot --query playa --id 1001 --min-price 40 --max-price 120 --top 5
```

Ejemplo en Windows:

```powershell
.\build\airbnb_indexer.exe --data data\pilot --query playa --id 1001 --min-price 40 --max-price 120 --top 5
```

## Parametros disponibles

| Parametro | Descripcion | Valor por defecto |
| --- | --- | --- |
| `--data` | Carpeta raiz donde se buscan CSV de forma recursiva | `data/pilot` |
| `--query` | Texto para busqueda parcial | `playa` |
| `--id` | ID de alojamiento para busqueda exacta | `1001` |
| `--min-price` | Precio minimo del filtro | `40` |
| `--max-price` | Precio maximo del filtro | `120` |
| `--top` | Numero maximo de resultados por seccion | `5` |
| `--export` | Ruta opcional para exportar resultados CSV | sin exportar |

## Uso con datasets reales

El prototipo espera archivos CSV compatibles con campos frecuentes de Airbnb/Inside Airbnb, por ejemplo:

- `id`
- `name`
- `host_id`
- `host_name`
- `neighbourhood`
- `room_type`
- `price`
- `minimum_nights`
- `number_of_reviews`
- `reviews_per_month`
- `availability_365`
- `latitude`
- `longitude`

Para probar con mas datos, colocar carpetas como:

```text
data/raw/lima/listings.csv
data/raw/lima/reviews.csv
data/raw/lima/calendar.csv
data/raw/cusco/listings.csv
```

Luego ejecutar:

```bash
./build/airbnb_indexer --data data/raw --query centro --min-price 50 --max-price 180
```

## Resultado esperado

El programa imprime:

- Archivos CSV encontrados por recorrido recursivo.
- Cantidad de listings cargados y filas omitidas.
- Tiempo de carga e indexacion.
- Estimacion aproximada de memoria usada por los indices.
- Resultado de busqueda exacta por ID.
- Resultados de busqueda parcial.
- Resultados filtrados por rango de precio.
- Rankings ordenados por precio y por puntaje.
- Resultados M4 de grafos: caminos minimos, MST, DFS y Tarjan.
- Resultados M5 de rangos: Segment Tree, Fenwick, busqueda binaria y actualizaciones dinamicas de calendario.

El documento del avance esta en `docs/AVANCE_2_GRUPO_03.md` y los resultados preliminares estan en `docs/RESULTADOS_PRELIMINARES.md`.
