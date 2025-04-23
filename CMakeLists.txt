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

# Aggiungi l'eseguibile principale
add_executable(negate main.cpp)

#ImageMagick
add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
pkg_check_modules(ImageMagick REQUIRED Magick++)
include_directories(${ImageMagick_INCLUDE_DIRS})
target_link_libraries(negate PRIVATE ${ImageMagick_LIBRARIES})
