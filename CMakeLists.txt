cmake_minimum_required(VERSION 3.20)
project(mp3_paralela CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Exportar compile_commands.json para clangd y herramientas de análisis
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Directorio de cabeceras del proyecto
include_directories(src)

# Fuentes del proyecto
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(mp3_paralela ${SOURCES} ${HEADERS})
