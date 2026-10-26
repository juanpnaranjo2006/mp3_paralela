cmake_minimum_required(VERSION 3.20)
project(mp3_paralela CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Exportar compile_commands.json para clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Directorios de inclusión
include_directories(src)
include_directories(raylib/include)
link_directories(raylib/lib)

# Fuentes comunes del núcleo de simulación
set(CORE_SOURCES
    src/simulator.cpp
    src/grid.cpp
)

# 1. Ejecutable de consola estilo UVA / Online Judge
add_executable(mp3_paralela
    src/main.cpp
    ${CORE_SOURCES}
)

# 2. Ejecutable gráfico interactivo con Raylib
add_executable(interactiva
    src/interactiva.cpp
    ${CORE_SOURCES}
)
target_link_libraries(interactiva PRIVATE raylib opengl32 gdi32 winmm)
