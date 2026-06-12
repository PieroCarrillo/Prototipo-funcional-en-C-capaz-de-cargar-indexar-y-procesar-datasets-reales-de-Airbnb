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

## Menu interactivo

La aplicacion presenta un menu de consola alineado con los modulos del
documento:

| Opcion | Contenido |
| --- | --- |
| M1 | Recorrido recursivo, clasificacion de archivos y resumen de carga |
| M2 | Construccion y metricas de los indices en memoria |
| M3 | Busqueda exacta y parcial, filtros, ordenamientos, ranking y exportacion |
| M4 | Caminos minimos, MST, DFS y Tarjan |
| M5 | Consultas por rangos y calendario dinamico |

Los listings y sus indices se preparan una sola vez al iniciar. El calendario
se carga bajo demanda al seleccionar M5, porque el archivo completo es
considerablemente mas grande.

## Ejecucion rapida

En Windows, desde VS Code, PowerShell o CMD:

```cmd
.\run.bat
```

Ese archivo compila y abre el menu usando
`C:\msys64\mingw64\bin\g++.exe`.

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
| `--graph-limit` | Maximo de listings usados por los algoritmos de grafos | `100` |
| `--export` | Ruta opcional para exportar resultados CSV | sin exportar |

## Dataset real de Paris

El archivo `Paris.zip` entregado por el profesor contiene:

- 82,467 registros en `listings.csv`.
- 2,281,438 registros resumidos en `reviews.csv`.
- 20 barrios en `neighbourhoods.csv`.
- 20 poligonos en `neighbourhoods.geojson`.
- Versiones detalladas o masivas comprimidas de listings, reviews y calendar.

Para importar los archivos resumidos desde la ubicacion predeterminada:

```cmd
.\import_paris.bat
```

Para importar desde otra ubicacion:

```cmd
.\import_paris.bat "C:\ruta\Paris.zip"
```

Para compilar y ejecutar directamente con Paris:

```cmd
.\run_paris.bat
```

Los datos se extraen en `data/real/Paris`, carpeta excluida de Git para evitar
subir cientos de megabytes al repositorio. La importacion descomprime los
30,100,525 registros de `calendar.csv`; se recomienda disponer de al menos
1.2 GB libres.

M5 recorre el calendario completo mediante streaming y acumula cada registro
por `listing_id`. De esta manera todos los registros son procesados, mientras
Segment Tree, Fenwick y AVL trabajan sobre una serie compacta por alojamiento.
En la entrega de Paris, `price` y `adjusted_price` estan vacios en el
calendario; por ello se acumula disponibilidad binaria: disponible = 1 y no
disponible = 0.

M4 usa todos los listings para formar los centroides de barrios. Los algoritmos
de caminos minimos sobre alojamientos mantienen el limite configurable
`--graph-limit`, porque Floyd-Warshall requiere O(V^3) operaciones y O(V^2)
memoria; ejecutarlo con 82,467 nodos no es viable. El menu aplica un maximo de
500 nodos y usa 100 de forma predeterminada.

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

Desde el menu se puede consultar:

- M1: archivos encontrados, filas cargadas y tiempo de lectura.
- M2: indices construidos, tiempo y memoria estimada.
- M3: busquedas, filtros, ordenamientos, ranking y exportacion.
- M4: caminos minimos, MST, DFS y Tarjan.
- M5: Segment Tree, Fenwick, busqueda binaria, AVL y calendario completo.

La validacion con la coleccion real del profesor esta documentada en
`docs/RESULTADOS_PARIS.md`.
