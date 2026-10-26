#include "grid.h"

Grid::Grid(size_t width, size_t height) {
  this->width = width;
  this->height = height;
  this->cells.resize(height, std::vector<Entity *>(width, nullptr));
}

void Grid::setEntities(
    std::vector<std::unique_ptr<Entity>> &entities,
    const std::vector<std::pair<std::size_t, std::size_t>> &positions) {
  for (size_t i = 0; i < entities.size(); i++) {
    this->cells[positions[i].second][positions[i].first] = entities[i].get();
  }
}

void Grid::addEntity(Entity *entity,
                     std::pair<std::size_t, std::size_t> position) {
  this->cells[position.second][position.first] = entity;
}

void Grid::moveEntity(std::pair<std::size_t, std::size_t> old_pos,
                      std::pair<std::size_t, std::size_t> new_pos) {
  this->cells[new_pos.second][new_pos.first] =
      this->cells[old_pos.second][old_pos.first];
  this->cells[old_pos.second][old_pos.first] = nullptr;
}

void Grid::removeEntity(std::pair<std::size_t, std::size_t> position) {
  this->cells[position.second][position.first] = nullptr;
}