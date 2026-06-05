# Avance 2: Desarrollo del prototipo algoritmico

## 1. Resumen

El segundo avance implementa un prototipo funcional en C++ para cargar, indexar y consultar datasets tipo Airbnb. La solucion recorre carpetas de forma recursiva, identifica archivos CSV, carga registros de alojamientos, construye indices en memoria y ejecuta busquedas exactas, busquedas parciales, filtros por rango y rankings. Tambien registra metricas preliminares de tiempo y memoria.

## 2. Objetivo del avance

Desarrollar un prototipo algoritimico inicial que demuestre la viabilidad tecnica del sistema propuesto en el primer avance, usando estructuras de datos en memoria RAM y operaciones medibles sobre colecciones piloto.

## 3. Alcance implementado

| Criterio | Implementacion |
| --- | --- |
| Recorrido de directorios en C++ | `DirectoryScanner` utiliza `std::filesystem::recursive_directory_iterator` para encontrar archivos CSV en subcarpetas. |
| Construccion del indice en memoria | `AirbnbIndex` construye indices por ID, texto, barrio y precio. |
| Busqueda exacta y parcial | Busqueda exacta con `unordered_map<long long, size_t>` y busqueda parcial con indice invertido de tokens. |
| Ordenamiento y filtrado | Filtro por rango de precio con `multimap<double, size_t>` y ranking con `priority_queue`. |
| Metricas de rendimiento | Se mide tiempo de carga, indexacion, busqueda exacta, busqueda textual, filtrado y ranking con `chrono`. |
| Simulacion piloto | `data/pilot/` contiene dos ciudades, archivos `listings.csv`, `reviews.csv` y `calendar.csv`. |

## 4. Arquitectura del prototipo

```text
Usuario CLI
   |
   v
main.cpp
   |
   +-- DirectoryScanner: recorrido recursivo e identificacion de CSV
   +-- DataLoader: lectura de listings desde CSV
   +-- CsvReader: parseo de lineas CSV con comillas
   +-- AirbnbIndex: indices en memoria y consultas
```

## 5. Estructuras de datos utilizadas

| Necesidad | Estructura | Motivo |
| --- | --- | --- |
| Busqueda exacta por ID | `unordered_map<long long, size_t>` | Permite acceso promedio O(1). |
| Busqueda textual parcial | `unordered_map<string, vector<size_t>>` | Implementa un indice invertido por token. |
| Consulta por precio | `multimap<double, size_t>` | Mantiene los alojamientos ordenados por precio y facilita rangos. |
| Ranking de mejores alojamientos | `priority_queue<size_t>` | Obtiene los elementos con mayor puntaje de forma eficiente. |
| Almacenamiento base | `vector<Listing>` | Conserva registros contiguos y referencia por posicion. |

## 6. Casos de prueba iniciales

Comando sugerido:

```bash
./build/airbnb_indexer --data data/pilot --query playa --id 1001 --min-price 40 --max-price 120 --top 5
```

Casos cubiertos:

| Caso | Entrada | Resultado esperado |
| --- | --- | --- |
| Busqueda exacta | `--id 1001` | Devuelve el departamento cerca a la playa en Miraflores. |
| Busqueda parcial | `--query playa` | Devuelve alojamientos cuyo nombre o zona contiene el token buscado. |
| Filtro por precio | `--min-price 40 --max-price 120` | Devuelve alojamientos dentro del rango indicado. |
| Ordenamiento | `--top 5` | Muestra los alojamientos mas baratos y el ranking por score. |
| Recorrido recursivo | `--data data/pilot` | Encuentra CSV dentro de `lima/` y `cusco/`. |

## 7. Metricas preliminares

El prototipo reporta en consola:

- Tiempo de carga de archivos CSV.
- Tiempo de construccion de indices.
- Tiempo de busqueda exacta.
- Tiempo de busqueda parcial.
- Tiempo de filtrado por rango.
- Tiempo de ordenamiento/ranking.
- Estimacion aproximada de memoria usada por los indices.

Estas metricas permiten comparar el enfoque indexado con recorridos secuenciales en el avance final.

Los resultados esperados sobre la coleccion piloto se documentan en `docs/RESULTADOS_PRELIMINARES.md`.

## 8. Limitaciones actuales

- La coleccion incluida es piloto y pequena para facilitar revision del codigo.
- Se cargan principalmente archivos `listings.csv`; `reviews.csv` y `calendar.csv` se detectan y quedan como base para integracion posterior.
- La normalizacion textual es basica y no elimina tildes; esto puede mejorarse en el informe final.
- La estimacion de memoria es aproximada porque las estructuras STL gestionan capacidad interna dependiente del compilador.

## 9. Trabajo pendiente para el informe final

- Integrar `reviews.csv` para busqueda masiva en comentarios.
- Agregar consultas por distancia geografica o grafo de barrios.
- Comparar tiempos con busqueda secuencial.
- Probar con colecciones reales de mayor volumen.
- Generar tablas comparativas y capturas del sistema.

## 10. Conclusiones del avance

El segundo avance demuestra que el sistema puede cargar archivos Airbnb desde carpetas, construir indices en memoria y responder consultas basicas de forma organizada. La separacion entre lectura, recorrido, carga e indexacion permite ampliar el prototipo hacia grafos, consultas por rango mas complejas y validacion con volumen real.
