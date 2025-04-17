cmake_minimum_required(VERSION 3.10)
project(negate CXX)

# Specifica il compilatore C++ da utilizzare
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "/usr/local/bin/g++-14" CACHE FILEPATH "C++ compiler")
endif()

# Imposta lo standard C++ a 23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trova pkg-config
find_package(PkgConfig REQUIRED)

# Usa pkg-config per trovare libpng
pkg_check_modules(PNG REQUIRED libpng)

# Per libraw, usa find_library poiché non ha un supporto pkg-config
find_library(RAW_LIBRARY raw REQUIRED)
find_path(RAW_INCLUDE_DIR libraw/libraw.h REQUIRED)

# Aggiungi l'eseguibile principale
add_executable(negate main.cpp)

# Includi i percorsi automaticamente rilevati per libpng
target_include_directories(negate PRIVATE ${PNG_INCLUDE_DIRS})
target_link_libraries(negate PRIVATE ${PNG_LIBRARIES})

# Aggiungi i percorsi di libraw
target_include_directories(negate PRIVATE ${RAW_INCLUDE_DIR})
target_link_libraries(negate PRIVATE ${RAW_LIBRARY})
