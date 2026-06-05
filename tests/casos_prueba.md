# Casos de prueba iniciales

## CP01 - Recorrido de directorios

Comando:

```bash
./build/airbnb_indexer --data data/pilot
```

Resultado esperado:

- Detecta archivos CSV dentro de `data/pilot/lima`.
- Detecta archivos CSV dentro de `data/pilot/cusco`.
- Clasifica `listings.csv`, `reviews.csv` y `calendar.csv`.

## CP02 - Busqueda exacta por ID

Comando:

```bash
./build/airbnb_indexer --data data/pilot --id 1001
```

Resultado esperado:

- Devuelve `Departamento luminoso cerca a la playa`.
- Muestra tiempo de busqueda exacta.

## CP03 - Busqueda parcial

Comando:

```bash
./build/airbnb_indexer --data data/pilot --query centro --top 5
```

Resultado esperado:

- Devuelve alojamientos de `Centro Historico` o alojamientos cuyo nombre contenga `centro/centrico`.
- Muestra tiempo de busqueda parcial.

## CP04 - Filtro por precio

Comando:

```bash
./build/airbnb_indexer --data data/pilot --min-price 40 --max-price 90 --top 10
```

Resultado esperado:

- Devuelve solo alojamientos con precio entre 40 y 90.
- Los resultados se obtienen desde el indice ordenado de precios.

## CP05 - Exportacion de ranking

Comando:

```bash
./build/airbnb_indexer --data data/pilot --export exports/ranking.csv
```

Resultado esperado:

- Crea un CSV con los alojamientos mejor rankeados.
- Incluye columnas de ID, nombre, barrio, tipo, precio, resenas, disponibilidad y score.
