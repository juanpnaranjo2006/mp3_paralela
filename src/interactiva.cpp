#include "entity.h"
#include "grid.h"
#include "simulator.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

// Estructura de historial para el gráfico estadístico de poblaciones
struct PopulationRecord {
  uint32_t step;
  uint64_t rabbits;
  uint64_t foxes;
};

// Función auxiliar para dibujar un botón interactivo
bool DrawSimpleButton(Rectangle bounds, const char *text, Color normalColor,
                      Color hoverColor, Color textColor) {
  Vector2 mousePos = GetMousePosition();
  bool hovered = CheckCollisionPointRec(mousePos, bounds);
  bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

  DrawRectangleRec(bounds, hovered ? hoverColor : normalColor);
  DrawRectangleLinesEx(bounds, 1, DARKGRAY);

  int fontSize = 16;
  int textWidth = MeasureText(text, fontSize);
  DrawText(text, (int)(bounds.x + (bounds.width - textWidth) / 2),
           (int)(bounds.y + (bounds.height - fontSize) / 2), fontSize,
           textColor);

  return clicked;
}

// Inicializar ecosistema con el caso de prueba oficial de 5x5
std::unique_ptr<Simulator> CreateUVASimulator(std::size_t &gridW,
                                              std::size_t &gridH) {
  gridW = 5;
  gridH = 5;
  uint32_t gen_proc_rabbits = 2;
  uint32_t gen_proc_foxes = 4;
  uint32_t gen_food_foxes = 3;

  std::vector<std::unique_ptr<Entity>> entities;
  std::vector<std::pair<std::size_t, std::size_t>> positions;

  auto addEntity = [&](EntityType type, std::size_t row, std::size_t col) {
    auto e = std::make_unique<Entity>();
    e->id = (uint32_t)entities.size();
    e->type = type;
    e->x = col;
    e->y = row;
    e->proc_age = 0;
    e->starve_age = 0;
    e->is_alive = true;
    e->is_new = false;
    entities.push_back(std::move(e));
    positions.push_back({col, row});
  };

  addEntity(EntityType::ROCK, 0, 0);
  addEntity(EntityType::RABBIT, 0, 2);
  addEntity(EntityType::FOX, 0, 4);
  addEntity(EntityType::FOX, 1, 0);
  addEntity(EntityType::FOX, 1, 4);
  addEntity(EntityType::ROCK, 2, 4);
  addEntity(EntityType::RABBIT, 3, 0);
  addEntity(EntityType::RABBIT, 4, 0);
  addEntity(EntityType::FOX, 4, 4);

  return std::make_unique<Simulator>(gridW, gridH, gen_proc_rabbits,
                                     gen_proc_foxes, gen_food_foxes, entities,
                                     positions);
}

// Inicializar un ecosistema aleatorio mediano (20x20) para apreciar ciclos Lotka-Volterra
std::unique_ptr<Simulator> CreateRandomSimulator(std::size_t w, std::size_t h,
                                                 int numRocks, int numRabbits,
                                                 int numFoxes,
                                                 uint32_t procRabbits,
                                                 uint32_t procFoxes,
                                                 uint32_t foodFoxes) {
  std::vector<std::unique_ptr<Entity>> entities;
  std::vector<std::pair<std::size_t, std::size_t>> positions;

  std::vector<std::pair<std::size_t, std::size_t>> allCells;
  for (std::size_t r = 0; r < h; ++r) {
    for (std::size_t c = 0; c < w; ++c) {
      allCells.push_back({c, r});
    }
  }

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(allCells.begin(), allCells.end(), g);

  std::size_t idx = 0;
  auto addBatch = [&](EntityType type, int count) {
    for (int i = 0; i < count && idx < allCells.size(); ++i, ++idx) {
      auto e = std::make_unique<Entity>();
      e->id = (uint32_t)entities.size();
      e->type = type;
      e->x = allCells[idx].first;
      e->y = allCells[idx].second;
      e->proc_age = 0;
      e->starve_age = 0;
      e->is_alive = true;
      e->is_new = false;
      positions.push_back(allCells[idx]);
      entities.push_back(std::move(e));
    }
  };

  addBatch(EntityType::ROCK, numRocks);
  addBatch(EntityType::RABBIT, numRabbits);
  addBatch(EntityType::FOX, numFoxes);

  return std::make_unique<Simulator>(w, h, procRabbits, procFoxes, foodFoxes,
                                     entities, positions);
}

int main() {
  const int screenWidth = 1150;
  const int screenHeight = 720;

  InitWindow(screenWidth, screenHeight,
             "Ecosystem Simulation - Interactive Visualizer (Raylib)");
  SetTargetFPS(60);

  std::size_t gridW = 5;
  std::size_t gridH = 5;
  std::unique_ptr<Simulator> simulator = CreateUVASimulator(gridW, gridH);

  // Parámetros de control de tiempo
  bool isPaused = true;
  float speedStepsPerSec = 4.0f; // Pasos de simulación por segundo
  float timeAccumulator = 0.0f;

  // Parámetros ajustables
  uint32_t procRabbits = simulator->getGenProcRabbits();
  uint32_t procFoxes = simulator->getGenProcFoxes();
  uint32_t foodFoxes = simulator->getGenFoodFoxes();

  // Historial de poblaciones para la gráfica
  std::vector<PopulationRecord> popHistory;
  popHistory.push_back({simulator->getStep(), simulator->getRabbitCount(),
                        simulator->getFoxCount()});

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // 1. Manejo de Controles de Tiempo y Teclado
    if (IsKeyPressed(KEY_SPACE)) {
      isPaused = !isPaused;
    }

    bool stepRequested = false;
    if (IsKeyPressed(KEY_S)) {
      stepRequested = true;
      isPaused = true;
    }

    if (!isPaused) {
      timeAccumulator += dt;
      float stepInterval = 1.0f / speedStepsPerSec;
      while (timeAccumulator >= stepInterval) {
        simulator->update();
        timeAccumulator -= stepInterval;
        popHistory.push_back({simulator->getStep(), simulator->getRabbitCount(),
                              simulator->getFoxCount()});
        if (popHistory.size() > 180) {
          popHistory.erase(popHistory.begin());
        }
      }
    } else if (stepRequested) {
      simulator->update();
      popHistory.push_back({simulator->getStep(), simulator->getRabbitCount(),
                            simulator->getFoxCount()});
      if (popHistory.size() > 180) {
        popHistory.erase(popHistory.begin());
      }
    }

    // 2. Renderizado
    BeginDrawing();
    ClearBackground(GetColor(0x18181bFF)); // Dark sleek background

    // --- PANEL IZQUIERDO: VISUALIZACIÓN DE LA GRILLA ---
    const float gridAreaX = 30.0f;
    const float gridAreaY = 30.0f;
    const float gridAreaSize = 660.0f;

    float cellW = gridAreaSize / (float)gridW;
    float cellH = gridAreaSize / (float)gridH;
    float cellSize = (cellW < cellH) ? cellW : cellH;

    float actualGridWidth = cellSize * (float)gridW;
    float actualGridHeight = cellSize * (float)gridH;

    // Fondo del tablero
    DrawRectangleRec({gridAreaX, gridAreaY, actualGridWidth, actualGridHeight},
                     GetColor(0x27272aFF));
    DrawRectangleLinesEx(
        {gridAreaX, gridAreaY, actualGridWidth, actualGridHeight}, 2,
        GetColor(0x3f3f46FF));

    Grid *currentGrid = simulator->getGrid();
    for (std::size_t r = 0; r < gridH; ++r) {
      for (std::size_t c = 0; c < gridW; ++c) {
        float cx = gridAreaX + (float)c * cellSize;
        float cy = gridAreaY + (float)r * cellSize;

        // Borde sutil de celda
        DrawRectangleLines((int)cx, (int)cy, (int)cellSize, (int)cellSize,
                           GetColor(0x3f3f46FF));

        Entity *e = (*currentGrid)[r][c];
        if (e != nullptr && e->is_alive) {
          float centerX = cx + cellSize / 2.0f;
          float centerY = cy + cellSize / 2.0f;
          float radius = cellSize * 0.38f;

          if (e->type == EntityType::ROCK) {
            DrawCircle((int)centerX, (int)centerY, radius, GRAY);
            DrawCircleLines((int)centerX, (int)centerY, radius, DARKGRAY);
            if (cellSize >= 20.0f) {
              DrawText("*", (int)(centerX - 4), (int)(centerY - 9), 18,
                       LIGHTGRAY);
            }
          } else if (e->type == EntityType::RABBIT) {
            DrawCircle((int)centerX, (int)centerY, radius,
                       GetColor(0x4ade80FF)); // Soft lime green
            DrawCircleLines((int)centerX, (int)centerY, radius, DARKGREEN);
            if (cellSize >= 20.0f) {
              DrawText("R", (int)(centerX - 5), (int)(centerY - 7), 16,
                       DARKGREEN);
            }
          } else if (e->type == EntityType::FOX) {
            DrawCircle((int)centerX, (int)centerY, radius,
                       GetColor(0xf87171FF)); // Warm fox red
            DrawCircleLines((int)centerX, (int)centerY, radius, MAROON);
            if (cellSize >= 20.0f) {
              DrawText("F", (int)(centerX - 5), (int)(centerY - 7), 16,
                       DARKGRAY);
            }
          }
        }
      }
    }

    // --- PANEL DERECHO: CONTROLES Y ESTADÍSTICAS ---
    float panelX = 720.0f;
    float currentY = 30.0f;

    DrawText("ECOSYSTEM SIMULATION", (int)panelX, (int)currentY, 22, RAYWHITE);
    currentY += 32.0f;

    // Sección de Estado y Poblaciones
    DrawRectangleRec({panelX, currentY, 390.0f, 105.0f}, GetColor(0x27272aFF));
    DrawRectangleLinesEx({panelX, currentY, 390.0f, 105.0f}, 1,
                         GetColor(0x3f3f46FF));

    DrawText(TextFormat("Generation: %u   Status: %s", simulator->getStep(),
                        isPaused ? "PAUSED" : "RUNNING"),
             (int)panelX + 15, (int)currentY + 12, 17,
             isPaused ? YELLOW : GREEN);

    DrawText(TextFormat("Rabbits (R): %llu", simulator->getRabbitCount()),
             (int)panelX + 15, (int)currentY + 38, 16, GetColor(0x4ade80FF));
    DrawText(TextFormat("Foxes (F):   %llu", simulator->getFoxCount()),
             (int)panelX + 15, (int)currentY + 60, 16, GetColor(0xf87171FF));
    DrawText(TextFormat("Rocks (*):   %llu", simulator->getRockCount()),
             (int)panelX + 15, (int)currentY + 82, 16, LIGHTGRAY);

    currentY += 120.0f;

    // Controles de Tiempo
    DrawText("Time Controls", (int)panelX, (int)currentY, 18, RAYWHITE);
    currentY += 25.0f;

    // Botones Play / Pause / Step
    if (DrawSimpleButton({panelX, currentY, 110.0f, 32.0f},
                         isPaused ? "Play (Space)" : "Pause (Space)",
                         isPaused ? GetColor(0x15803dFF) : GetColor(0xb91c1cFF),
                         isPaused ? GetColor(0x16a34aFF) : GetColor(0xdc2626FF),
                         RAYWHITE)) {
      isPaused = !isPaused;
    }

    if (DrawSimpleButton({panelX + 120.0f, currentY, 80.0f, 32.0f}, "Step (S)",
                         GetColor(0x3b82f6FF), GetColor(0x60a5faFF),
                         RAYWHITE)) {
      simulator->update();
      popHistory.push_back({simulator->getStep(), simulator->getRabbitCount(),
                            simulator->getFoxCount()});
      if (popHistory.size() > 180) {
        popHistory.erase(popHistory.begin());
      }
    }

    // Control de Velocidad
    if (DrawSimpleButton({panelX + 220.0f, currentY, 40.0f, 32.0f}, "-",
                         GetColor(0x3f3f46FF), GetColor(0x52525bFF),
                         RAYWHITE)) {
      if (speedStepsPerSec > 1.0f)
        speedStepsPerSec -= 1.0f;
    }
    DrawText(TextFormat("%2.0f gen/s", speedStepsPerSec), (int)panelX + 270,
             (int)currentY + 7, 15, LIGHTGRAY);
    if (DrawSimpleButton({panelX + 345.0f, currentY, 40.0f, 32.0f}, "+",
                         GetColor(0x3f3f46FF), GetColor(0x52525bFF),
                         RAYWHITE)) {
      if (speedStepsPerSec < 60.0f)
        speedStepsPerSec += 2.0f;
    }

    currentY += 45.0f;

    // Sección de Parámetros Ajustables
    DrawText("Adjustable Parameters", (int)panelX, (int)currentY, 18, RAYWHITE);
    currentY += 25.0f;

    auto drawParamControl = [&](const char *label, uint32_t &paramVal,
                                void (Simulator::*setter)(uint32_t)) {
      DrawText(label, (int)panelX + 10, (int)currentY + 6, 15, LIGHTGRAY);
      if (DrawSimpleButton({panelX + 220.0f, currentY, 32.0f, 26.0f}, "-",
                           GetColor(0x3f3f46FF), GetColor(0x52525bFF),
                           RAYWHITE)) {
        if (paramVal > 1) {
          paramVal--;
          (simulator.get()->*setter)(paramVal);
        }
      }
      DrawText(TextFormat("%2u", paramVal), (int)panelX + 265,
               (int)currentY + 5, 16, RAYWHITE);
      if (DrawSimpleButton({panelX + 295.0f, currentY, 32.0f, 26.0f}, "+",
                           GetColor(0x3f3f46FF), GetColor(0x52525bFF),
                           RAYWHITE)) {
        if (paramVal < 99) {
          paramVal++;
          (simulator.get()->*setter)(paramVal);
        }
      }
      currentY += 32.0f;
    };

    drawParamControl("Rabbit Procreation (GEN_PROC)", procRabbits,
                     &Simulator::setGenProcRabbits);
    drawParamControl("Fox Procreation    (GEN_PROC)", procFoxes,
                     &Simulator::setGenProcFoxes);
    drawParamControl("Fox Food Limit     (GEN_FOOD)", foodFoxes,
                     &Simulator::setGenFoodFoxes);

    currentY += 10.0f;

    // Botones de presets / reset
    if (DrawSimpleButton({panelX, currentY, 185.0f, 32.0f}, "Reset UVA (5x5)",
                         GetColor(0x3f3f46FF), GetColor(0x52525bFF),
                         RAYWHITE)) {
      simulator = CreateUVASimulator(gridW, gridH);
      procRabbits = simulator->getGenProcRabbits();
      procFoxes = simulator->getGenProcFoxes();
      foodFoxes = simulator->getGenFoodFoxes();
      popHistory.clear();
      popHistory.push_back({simulator->getStep(), simulator->getRabbitCount(),
                            simulator->getFoxCount()});
      isPaused = true;
    }

    if (DrawSimpleButton({panelX + 195.0f, currentY, 195.0f, 32.0f},
                         "Random Ecosystem (20x20)", GetColor(0x3f3f46FF),
                         GetColor(0x52525bFF), RAYWHITE)) {
      gridW = 20;
      gridH = 20;
      procRabbits = 3;
      procFoxes = 6;
      foodFoxes = 5;
      simulator = CreateRandomSimulator(gridW, gridH, 20, 60, 25, procRabbits,
                                        procFoxes, foodFoxes);
      popHistory.clear();
      popHistory.push_back({simulator->getStep(), simulator->getRabbitCount(),
                            simulator->getFoxCount()});
      isPaused = true;
    }

    currentY += 45.0f;

    // --- GRÁFICA DE POBLACIÓN (Statistics Tracking over time) ---
    DrawText("Population Dynamics (Lotka-Volterra Cycles)", (int)panelX,
             (int)currentY, 16, RAYWHITE);
    currentY += 22.0f;

    Rectangle chartRect = {panelX, currentY, 390.0f, 150.0f};
    DrawRectangleRec(chartRect, GetColor(0x1f1f23FF));
    DrawRectangleLinesEx(chartRect, 1, GetColor(0x3f3f46FF));

    // Determinar escala máxima
    uint64_t maxPop = 10;
    for (const auto &p : popHistory) {
      if (p.rabbits > maxPop)
        maxPop = p.rabbits;
      if (p.foxes > maxPop)
        maxPop = p.foxes;
    }

    // Dibujar curvas de población
    if (popHistory.size() >= 2) {
      for (size_t i = 0; i < popHistory.size() - 1; ++i) {
        float x1 = chartRect.x +
                   ((float)i / (float)(popHistory.size() - 1)) * chartRect.width;
        float x2 = chartRect.x + ((float)(i + 1) /
                                  (float)(popHistory.size() - 1)) *
                                     chartRect.width;

        float yr1 = chartRect.y + chartRect.height -
                    ((float)popHistory[i].rabbits / (float)maxPop) *
                        (chartRect.height - 10.0f) -
                    5.0f;
        float yr2 = chartRect.y + chartRect.height -
                    ((float)popHistory[i + 1].rabbits / (float)maxPop) *
                        (chartRect.height - 10.0f) -
                    5.0f;

        float yf1 = chartRect.y + chartRect.height -
                    ((float)popHistory[i].foxes / (float)maxPop) *
                        (chartRect.height - 10.0f) -
                    5.0f;
        float yf2 = chartRect.y + chartRect.height -
                    ((float)popHistory[i + 1].foxes / (float)maxPop) *
                        (chartRect.height - 10.0f) -
                    5.0f;

        DrawLineEx({x1, yr1}, {x2, yr2}, 2.0f, GetColor(0x4ade80FF));
        DrawLineEx({x1, yf1}, {x2, yf2}, 2.0f, GetColor(0xf87171FF));
      }
    }

    // Leyenda de la gráfica
    DrawText("--- Rabbits", (int)chartRect.x + 10, (int)chartRect.y + 8, 14,
             GetColor(0x4ade80FF));
    DrawText("--- Foxes", (int)chartRect.x + 110, (int)chartRect.y + 8, 14,
             GetColor(0xf87171FF));
    DrawText(TextFormat("Max: %llu", maxPop), (int)chartRect.x + 310,
             (int)chartRect.y + 8, 14, LIGHTGRAY);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
