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

echo Extrayendo archivos de Paris...
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

echo Descomprimiendo el calendario completo...
echo Este proceso requiere aproximadamente 1.2 GB libres y puede tardar.
powershell -NoProfile -ExecutionPolicy Bypass ^
  -File "scripts\extract_gzip_csv.ps1" ^
  -InputPath "%TARGET%\Paris\calendar.csv.gz" ^
  -OutputPath "%TARGET%\Paris\calendar.csv"

if errorlevel 1 (
  echo Error al descomprimir el calendario.
  exit /b 1
)

del "%TARGET%\Paris\calendar.csv.gz"
> "%TARGET%\Paris\.full-dataset-ready" echo Paris completo

echo.
echo Dataset disponible en data\real\Paris
echo calendar.csv contiene todos los registros entregados por el profesor.
echo Los archivos comprimidos detallados restantes permanecen dentro del ZIP.
