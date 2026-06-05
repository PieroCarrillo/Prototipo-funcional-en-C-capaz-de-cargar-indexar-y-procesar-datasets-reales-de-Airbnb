cmake_minimum_required(VERSION 3.15)
project(airbnb_indexer_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(airbnb_indexer
    src/main.cpp
    src/AirbnbIndex.cpp
    src/CsvReader.cpp
    src/DataLoader.cpp
    src/DirectoryScanner.cpp
)

target_include_directories(airbnb_indexer PRIVATE include)

