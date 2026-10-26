#ifndef GRID_H
#define GRID_H

#include "entity.h"
#include <cstddef> // Para std::size_t
#include <memory>
#include <utility>
#include <vector>

class Grid {
private:
  std::size_t width{0};
  std::size_t height{0};

  std::vector<std::vector<Entity *>> cells;

public:
  Grid() = default;
  Grid(std::size_t width, std::size_t height);

  ~Grid() = default;

  // Regla de los 5: Operaciones de copia habilitadas explícitamente
  Grid(const Grid &) = default;
  Grid &operator=(const Grid &) = default;

  // Operaciones de movimiento habilitadas explícitamente
  Grid(Grid &&) noexcept = default;
  Grid &operator=(Grid &&) noexcept = default;

  // Setters
  void setEntities(
      std::vector<std::unique_ptr<Entity>> &entities,
      const std::vector<std::pair<std::size_t, std::size_t>> &positions);

  // Getters
  std::size_t getWidth() const { return this->width; }
  std::size_t getHeight() const { return this->height; }

  // SOBRECARGA DEL OPERADOR []
  std::vector<Entity *> &operator[](std::size_t y) { return this->cells[y]; }

  const std::vector<Entity *> &operator[](std::size_t y) const {
    return this->cells[y];
  }

  // Other methods
  void addEntity(Entity *entity, std::pair<std::size_t, std::size_t> position);

  void moveEntity(std::pair<std::size_t, std::size_t> old_pos,
                  std::pair<std::size_t, std::size_t> new_pos);

  void removeEntity(std::pair<std::size_t, std::size_t> position);
};

#endif