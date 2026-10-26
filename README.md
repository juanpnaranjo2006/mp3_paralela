# Simulación de Ecosistema (Ecosystem Simulation)

Este proyecto implementa un simulador generacional de un ecosistema compuesto por **conejos** (presas), **zorros** (depredadores) y **rocas** (obstáculos ambientales), siguiendo dinámicas de población basadas en autómatas celulares y modelos depredador-presa (tipo Lotka-Volterra).

> **Nota sobre esta rama:** Esta rama contiene la **versión secuencial** del proyecto. Sirve como base algorítmica y de validación antes de desarrollar la versión paralela en memoria compartida (OpenMP / Pthreads).

---

## Estado de la Simulación y Casos de Prueba

- **Caso de prueba oficial (5x5):** Coincide al **100%** tanto en el resultado final como en cada una de las generaciones intermedias descritas en el documento de la asignación.
- **Casos de prueba extendidos (10x10, 20x20):** El motor replica con precisión las primeras generaciones. Las divergencias que aparecen en generaciones avanzadas se deben a detalles de especificación no aclarados en el enunciado formal (por ejemplo, si una cría de conejo recién nacida puede ser cazada en la misma generación en la que nace o si cuenta con protección temporal, o el criterio de desempate estricto cuando dos zorros colisionan teniendo exactamente la misma edad de procreación e inanición). En términos generales, el simulador es sumamente sólido, respeta las reglas y reproduce fielmente los ciclos biológicos esperados.

---

## Requisitos y Dependencias

- **Compilador C++:** Soporte para C++20 (`g++` / MinGW-w64 / MSYS2 UCRT64).
- **Herramienta de construcción (opcional):** CMake 3.20 o superior.
- **Librería gráfica:** [Raylib](https://www.raylib.com/) v5.0 (incluida en el proyecto para Windows en la carpeta `raylib/`).

---

## Estructura del Código

```text
mp3_paralela/
├── CMakeLists.txt        # Configuración de CMake para ambos ejecutables
├── ecosystem_examples/   # Casos de prueba y trazas de referencia del profesor
├── raylib/               # Librería y cabeceras de Raylib para la GUI
└── src/
    ├── entity.h          # Definición de la estructura Entity
    ├── grid.h / .cpp     # Estructura espacial 2D del mundo
    ├── simulator.h / .cpp # Núcleo del motor de simulación y resolución de conflictos
    ├── main.cpp          # Versión de consola (I/O estándar estilo UVA)
    └── interactiva.cpp   # Versión interactiva gráfica con Raylib
```

---

## 1. Versión de Consola (Estilo Juez en Línea / UVA)

Diseñada para leer desde la entrada estándar (`stdin`) y escribir el resultado final en la salida estándar (`stdout`).

### Compilación

**Opción A (Compilación directa con `g++`):**
```powershell
g++ -std=c++20 -O2 src/main.cpp src/simulator.cpp src/grid.cpp -Isrc -o mp3_paralela.exe
```

**Opción B (Usando CMake):**
```powershell
cmake -B build -G "MinGW Makefiles"
cmake --build build --target mp3_paralela
```

### Ejecución

Puedes pasar un archivo de entrada mediante redirección:
```powershell
.\mp3_paralela.exe < .\ecosystem_examples\input5x5
```

O guardar la salida en un archivo de texto:
```powershell
.\mp3_paralela.exe < .\ecosystem_examples\input20x20 > .\results\output20x20.txt
```

#### Formato de Entrada
```text
GEN_PROC_RABBITS GEN_PROC_FOXES GEN_FOOD_FOXES N_GEN R C N
OBJECT ROW COL
...
```

#### Formato de Salida
```text
GEN_PROC_RABBITS GEN_PROC_FOXES GEN_FOOD_FOXES 0 R C N_FINAL
OBJECT ROW COL
...
```

---

## 2. Versión Gráfica Interactiva (Raylib)

Proporciona una interfaz visual que cumple con todos los requisitos de la sección gráfica del proyecto:
- **Grilla visual:** Renderizado dinámico de celdas, rocas ($\star$), conejos (verde) y zorros (rojo).
- **Seguimiento estadístico en tiempo real:** Gráfica continua de poblaciones que visualiza los ciclos presa-depredador.
- **Controles de tiempo:** Pausar, reanudar, avanzar paso a paso (*step*) y ajustar la velocidad (1 a 60 gen/s).
- **Parámetros ajustables en vivo:** Incremento o reducción de tiempos de procreación e inanición.
- **Presets:** Botones para recargar el caso de prueba 5x5 o generar un ecosistema aleatorio de 20x20.

### Compilación

**Opción A (Compilación directa con `g++`):**
```powershell
g++ -std=c++20 -O2 src/interactiva.cpp src/simulator.cpp src/grid.cpp -Isrc -Iraylib/include -Lraylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm -o interactiva.exe
```

**Opción B (Usando CMake):**
```powershell
cmake -B build -G "MinGW Makefiles"
cmake --build build --target interactiva
```

### Ejecución y Controles

```powershell
.\interactiva.exe
```

| Control | Acción |
| :--- | :--- |
| **Espacio (`Space`)** | Alternar entre Pausa y Reproducción continua |
| **Tecla `S` / Botón `Step`** | Avanzar exactamente 1 generación (modo paso a paso) |
| **Botones `+` / `-` (Velocidad)** | Aumentar o disminuir la tasa de simulación (gen/s) |
| **Botones `+` / `-` (Parámetros)** | Modificar `GEN_PROC_RABBITS`, `GEN_PROC_FOXES` o `GEN_FOOD_FOXES` en vivo |
| **Botón `Reset UVA (5x5)`** | Reiniciar al caso de prueba de 5x5 del enunciado |
| **Botón `Random Ecosystem (20x20)`** | Generar una cuadrícula ampliada de 20x20 con múltiples especies |
