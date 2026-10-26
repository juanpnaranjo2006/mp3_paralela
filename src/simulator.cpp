#include "simulator.h"
#include "entity.h"
#include <utility>

Simulator::Simulator(
    std::size_t width, std::size_t height,
    std::vector<std::unique_ptr<Entity>> &entities,
    std::vector<std::pair<std::size_t, std::size_t>> &positions) {
  this->entities = std::move(entities);
  this->step = 0;

  this->grid = new Grid(width, height);
  this->grid->setEntities(this->entities, positions);
  this->next_grid = new Grid(width, height);

  this->count_rock = 0;
  this->count_rabbit = 0;
  this->count_fox = 0;

  for (auto &entity : this->entities) {
    switch (entity->type) {
    case EntityType::ROCK:
      this->rocks.push_back(entity.get());
      this->count_rock++;
      break;
    case EntityType::RABBIT:
      this->rabbits.push_back(entity.get());
      this->count_rabbit++;
      break;
    case EntityType::FOX:
      this->foxes.push_back(entity.get());
      this->count_fox++;
      break;
    }
  }
}

bool Simulator::isMoveable(std::size_t x, std::size_t y) {
  return !((*(this->grid))[y][x]->type == EntityType::ROCK);
}

bool Simulator::isRabbit(std::size_t x, std::size_t y) {
  return ((*(this->grid))[y][x]->type == EntityType::RABBIT);
}

bool Simulator::isFox(std::size_t x, std::size_t y) {
  return ((*(this->grid))[y][x]->type == EntityType::FOX);
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getPossibleMovesRabbit(std::size_t x, std::size_t y) {
  size_t h = (*(this->grid)).getHeight();
  size_t w = (*(this->grid)).getWidth();

  std::vector<std::pair<std::size_t, std::size_t>> possible_moves;
  /*By following the clockwise order, start numbering from 0 the possible
    P cells to where a fox/rabbit can move (adjacent north, east, south
    and west cells).
  */
  if (y < h - 1 && isMoveable(x, y + 1) && !isFox(x, y + 1))
    possible_moves.push_back(std::make_pair(x, y + 1));
  if (x < w - 1 && isMoveable(x + 1, y) && !isFox(x + 1, y))
    possible_moves.push_back(std::make_pair(x + 1, y));
  if (y > 0 && isMoveable(x, y - 1) && !isFox(x, y - 1))
    possible_moves.push_back(std::make_pair(x, y - 1));
  if (x > 0 && isMoveable(x - 1, y) && !isFox(x - 1, y))
    possible_moves.push_back(std::make_pair(x - 1, y));

  return possible_moves;
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getPossibleMovesFox(std::size_t x, std::size_t y) {
  size_t h = (*(this->grid)).getHeight();
  size_t w = (*(this->grid)).getWidth();

  std::vector<std::pair<std::size_t, std::size_t>> possible_moves;
  /*If there are any rabbits in one of the 4 adjacent cells, the fox will
  prioritize eating them instead of moving.
  */
  /*By following the clockwise order, start numbering from 0 the possible
    P cells to where a fox/rabbit can move (adjacent north, east, south
    and west cells).
  */
  if (y < h - 1 && isRabbit(x, y + 1))
    possible_moves.push_back(std::make_pair(x, y + 1));
  if (x < w - 1 && isRabbit(x + 1, y))
    possible_moves.push_back(std::make_pair(x + 1, y));
  if (y > 0 && isRabbit(x, y - 1))
    possible_moves.push_back(std::make_pair(x, y - 1));
  if (x > 0 && isRabbit(x - 1, y))
    possible_moves.push_back(std::make_pair(x - 1, y));

  if (!possible_moves.empty()) {
    return possible_moves;
  }

  if (y < h - 1 && isMoveable(x, y + 1))
    possible_moves.push_back(std::make_pair(x, y + 1));
  if (x < w - 1 && isMoveable(x + 1, y))
    possible_moves.push_back(std::make_pair(x + 1, y));
  if (y > 0 && isMoveable(x, y - 1))
    possible_moves.push_back(std::make_pair(x, y - 1));
  if (x > 0 && isMoveable(x - 1, y))
    possible_moves.push_back(std::make_pair(x - 1, y));

  return possible_moves;
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getPossibleMoves(std::size_t x, std::size_t y) {
  EntityType entity_type = (*(this->grid))[y][x]->type;

  if (entity_type == EntityType::RABBIT) {
    return this->getPossibleMovesRabbit(x, y);
  } else if (entity_type == EntityType::FOX) {
    return this->getPossibleMovesFox(x, y);
  }
  return std::vector<std::pair<std::size_t, std::size_t>>();
}

std::pair<int, int> Simulator::selectDisplacement(
    std::uint32_t g, std::size_t x, std::size_t y,
    std::vector<std::pair<std::size_t, std::size_t>> &possible_moves) {
  std::size_t pos = (g + x + y) % possible_moves.size();

  return possible_moves[pos];
}

std::pair<std::size_t, std::size_t> Simulator::chooseAdjCell(std::size_t x,
                                                             std::size_t y) {
  std::vector<std::pair<std::size_t, std::size_t>> possible_moves =
      this->getPossibleMoves(x, y);
  if (possible_moves.empty()) {
    return std::make_pair(x, y);
  }

  std::pair<int, int> displacement =
      this->selectDisplacement(this->step, x, y, possible_moves);

  return std::make_pair(x + displacement.first, y + displacement.second);
}

// Update at each simulation step
// SECUENTIAL VERSION

void Simulator::update() {
  size_t h = this->grid->getHeight();
  size_t w = this->grid->getWidth();

  // Update rocks
  for (auto &rock : this->rocks) {
    this->next_grid->addEntity(rock, std::make_pair(rock->x, rock->y));
  }

  size_t prev_x;
  size_t prev_y;

  // Update rabbits
  for (auto &rabbit : this->rabbits) {
    if (!rabbit->is_alive || rabbit->is_new) {
      rabbit->is_new = false;
      continue;
    }
    prev_x = rabbit->x;
    prev_y = rabbit->y;

    std::pair<std::size_t, std::size_t> new_pos =
        chooseAdjCell(rabbit->x, rabbit->y);

    if (new_pos.first == prev_x && new_pos.second == prev_y) {
      this->next_grid->addEntity(rabbit, new_pos);
      rabbit->proc_age++;
      continue;
    }

    Entity *neighbor = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (neighbor == nullptr || (neighbor->type == EntityType::RABBIT &&
                                neighbor->proc_age < rabbit->proc_age)) {
      neighbor->is_alive = false;
      count_rabbit--;

      this->next_grid->addEntity(rabbit, new_pos);
      rabbit->x = new_pos.first;
      rabbit->y = new_pos.second;
    }

    // Procreate and update ages
    if (rabbit->proc_age >= GEN_PROC_RABBITS) {
      rabbit->proc_age = 0;

      std::unique_ptr<Entity> child = std::make_unique<Entity>();
      child->type = EntityType::RABBIT;
      child->id = this->entities.size();
      child->x = prev_x;
      child->y = prev_y;
      child->proc_age = 0;
      child->starve_age = 0;
      child->is_alive = true;
      child->is_new = true;
      this->entities.push_back(std::move(child));
      this->rabbits.push_back(this->entities.back().get());
      this->next_grid->addEntity(child.get(), std::make_pair(prev_x, prev_y));
    } else {
      rabbit->proc_age++;
    }
  }

  // Update foxes
  for (auto &fox : this->foxes) {
    if (!fox->is_alive || fox->is_new) {
      fox->is_new = false;
      continue;
    }
    prev_x = fox->x;
    prev_y = fox->y;
    std::pair<std::size_t, std::size_t> new_pos = chooseAdjCell(fox->x, fox->y);

    if (new_pos.first == prev_x && new_pos.second == prev_y) {
      if (fox->starve_age >= GEN_FOOD_FOXES) {
        fox->is_alive = false;
        count_fox--;
        continue;
      }
      this->next_grid->addEntity(fox, new_pos);
      fox->starve_age++;
      continue;
    }

    Entity *neighbor = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (neighbor == nullptr && fox->starve_age < GEN_FOOD_FOXES) {
      this->next_grid->addEntity(fox, new_pos);
      fox->x = new_pos.first;
      fox->y = new_pos.second;
      fox->starve_age++;
    } else if (neighbor != nullptr && neighbor->type == EntityType::RABBIT) {
      neighbor->is_alive = false;
      count_rabbit--;

      this->next_grid->addEntity(fox, new_pos);
      fox->x = new_pos.first;
      fox->y = new_pos.second;
      fox->starve_age = 0;
    } else if (fox->starve_age >= GEN_FOOD_FOXES) {
      fox->is_alive = false;
      count_fox--;
      continue;
    } else if (neighbor->type == EntityType::FOX &&
               (neighbor->proc_age < fox->proc_age ||
                (neighbor->proc_age == fox->proc_age &&
                 neighbor->starve_age > fox->starve_age))) {
      neighbor->is_alive = false;
      count_fox--;

      this->next_grid->addEntity(fox, new_pos);
      fox->x = new_pos.first;
      fox->y = new_pos.second;
      fox->starve_age++;
    }

    // If fox survived, procreate and update ages
    if (fox->is_alive && fox->proc_age >= GEN_PROC_FOXES) {
      fox->proc_age = 0;

      std::unique_ptr<Entity> child = std::make_unique<Entity>();
      child->type = EntityType::FOX;
      child->id = this->entities.size();
      child->x = prev_x;
      child->y = prev_y;
      child->proc_age = 0;
      child->starve_age = 0;
      child->is_alive = true;
      child->is_new = true;
      this->entities.push_back(std::move(child));
      this->foxes.push_back(this->entities.back().get());
      this->next_grid->addEntity(child.get(), std::make_pair(prev_x, prev_y));
    }
  }

  this->grid = this->next_grid;
  this->next_grid = new Grid(w, h);
}
