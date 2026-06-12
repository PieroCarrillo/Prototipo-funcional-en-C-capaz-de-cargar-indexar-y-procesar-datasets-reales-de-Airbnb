@echo off
setlocal

set "ZIP=%~1"
if "%ZIP%"=="" set "ZIP=C:\Users\piero\Downloads\Paris.zip"
set "TARGET=data\real"

if not exist "%ZIP%" (
  echo No se encontro el archivo:
  echo %ZIP%
  echo.
  echo Uso: import_paris.bat "C:\ruta\Paris.zip"
  exit /b 1
)

if not exist "%TARGET%" mkdir "%TARGET%"

echo Extrayendo archivos resumidos de Paris...
tar -xf "%ZIP%" -C "%TARGET%" ^
  Paris/calendar.csv.gz ^
  Paris/listings.csv ^
  Paris/reviews.csv ^
  Paris/neighbourhoods.csv ^
  Paris/neighbourhoods.geojson

if errorlevel 1 (
  echo Error al extraer el dataset.
  exit /b 1
)

echo Creando muestra de 100000 filas de calendar.csv.gz...
powershell -NoProfile -ExecutionPolicy Bypass ^
  -File "scripts\extract_gzip_sample.ps1" ^
  -InputPath "%TARGET%\Paris\calendar.csv.gz" ^
  -OutputPath "%TARGET%\Paris\calendar.csv" ^
  -MaxDataRows 100000 ^
  -RequiredToken ",t,"

if errorlevel 1 (
  echo Error al crear la muestra de calendario.
  exit /b 1
)

del "%TARGET%\Paris\calendar.csv.gz"

echo.
echo Dataset disponible en data\real\Paris
echo Se creo calendar.csv con una muestra real de 100000 filas.
echo Los archivos detallados restantes permanecen dentro del ZIP.
