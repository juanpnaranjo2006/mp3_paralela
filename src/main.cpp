#include "entity.h"
#include "simulator.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

int main() {
  // Optimización de I/O estándar de C++ estilo UVA/CP
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  std::uint32_t gen_proc_rabbits;
  std::uint32_t gen_proc_foxes;
  std::uint32_t gen_food_foxes;
  std::uint32_t n_gen;
  std::size_t r;
  std::size_t c;
  std::size_t n;

  // Leer la línea de configuración
  if (!(std::cin >> gen_proc_rabbits >> gen_proc_foxes >> gen_food_foxes >>
        n_gen >> r >> c >> n)) {
    return 0;
  }

  std::vector<std::unique_ptr<Entity>> entities;
  std::vector<std::pair<std::size_t, std::size_t>> positions;
  entities.reserve(n);
  positions.reserve(n);

  // Leer los N objetos iniciales del ecosistema
  for (std::size_t i = 0; i < n; ++i) {
    std::string type_str;
    std::size_t row;
    std::size_t col;
    std::cin >> type_str >> row >> col;

    auto entity = std::make_unique<Entity>();
    entity->id = static_cast<std::uint32_t>(i);
    entity->x = col;
    entity->y = row;
    entity->proc_age = 0;
    entity->starve_age = 0;
    entity->is_alive = true;
    entity->is_new = false;

    if (type_str == "ROCK") {
      entity->type = EntityType::ROCK;
    } else if (type_str == "RABBIT") {
      entity->type = EntityType::RABBIT;
    } else if (type_str == "FOX") {
      entity->type = EntityType::FOX;
    }

    entities.push_back(std::move(entity));
    positions.push_back(std::make_pair(col, row));
  }

  // Inicializar el simulador: width = c, height = r
  Simulator simulator(c, r, gen_proc_rabbits, gen_proc_foxes, gen_food_foxes,
                      entities, positions);

  // Ejecutar la simulación durante N_GEN generaciones
  for (std::uint32_t g = 0; g < n_gen; ++g) {
    simulator.update();
  }

  // Recolectar el estado final del ecosistema recorriendo fila por fila
  Grid *final_grid = simulator.getGrid();
  std::vector<std::tuple<std::string, std::size_t, std::size_t>> final_objects;

  for (std::size_t row = 0; row < r; ++row) {
    for (std::size_t col = 0; col < c; ++col) {
      Entity *entity = (*final_grid)[row][col];
      if (entity != nullptr && entity->is_alive) {
        std::string name;
        switch (entity->type) {
        case EntityType::ROCK:
          name = "ROCK";
          break;
        case EntityType::RABBIT:
          name = "RABBIT";
          break;
        case EntityType::FOX:
          name = "FOX";
          break;
        }
        final_objects.emplace_back(name, row, col);
      }
    }
  }

  // Imprimir cabecera de salida con el número final de generaciones restantes (0)
  // y la cantidad de objetos finales
  std::cout << gen_proc_rabbits << " " << gen_proc_foxes << " "
            << gen_food_foxes << " " << 0 << " " << r << " " << c << " "
            << final_objects.size() << "\n";

  // Imprimir los objetos finales
  for (const auto &[name, row, col] : final_objects) {
    std::cout << name << " " << row << " " << col << "\n";
  }

  return 0;
}
