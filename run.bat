@echo off
setlocal

set "GPP=C:\msys64\mingw64\bin\g++.exe"
set "EXE=build\airbnb_indexer.exe"
set "PATH=C:\msys64\mingw64\bin;%PATH%"

if not exist "%GPP%" (
  echo No se encontro g++ en "%GPP%".
  echo Abre MSYS2 MINGW64 o agrega C:\msys64\mingw64\bin al PATH.
  exit /b 1
)

if not exist build mkdir build

echo Compilando...
"%GPP%" -std=c++17 -O2 -Iinclude src\AirbnbIndex.cpp src\CsvReader.cpp src\DataLoader.cpp src\DirectoryScanner.cpp src\main.cpp -o "%EXE%"
if errorlevel 1 (
  echo Error al compilar.
  exit /b 1
)

echo.
echo Ejecutando...
"%EXE%" --data data\pilot --query playa --id 1001 --min-price 40 --max-price 120 --top 5
