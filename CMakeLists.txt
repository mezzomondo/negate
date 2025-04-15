cmake_minimum_required(VERSION 3.10)
project(negate CXX) # Cambiato da C a CXX per indicare che usiamo C++

# Setta lo standard di C++ (ad esempio C++11, C++14, C++17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Aggiungi l'eseguibile principale
add_executable(negate main.cpp)

# Specifica i percorsi delle librerie e file di intestazione
include_directories(/usr/local/include/libraw)
link_directories(/usr/local/lib)

# Linka la libreria LibRaw
target_link_libraries(negate raw)
