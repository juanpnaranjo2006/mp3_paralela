#include "simulator.h"
#include "entity.h"
#include <utility>

Simulator::Simulator(
    std::size_t width, std::size_t height,
    std::vector<std::unique_ptr<Entity>> &entities,
    std::vector<std::pair<std::size_t, std::size_t>> &positions)
    : Simulator(width, height, 0, 0, 0, entities, positions) {}

Simulator::Simulator(
    std::size_t width, std::size_t height, std::uint32_t gen_proc_rabbits,
    std::uint32_t gen_proc_foxes, std::uint32_t gen_food_foxes,
    std::vector<std::unique_ptr<Entity>> &entities,
    std::vector<std::pair<std::size_t, std::size_t>> &positions) {
  this->GEN_PROC_RABBITS = gen_proc_rabbits;
  this->GEN_PROC_FOXES = gen_proc_foxes;
  this->GEN_FOOD_FOXES = gen_food_foxes;
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

Simulator::~Simulator() {
  delete this->grid;
  delete this->next_grid;
}

bool Simulator::isEmpty(std::size_t x, std::size_t y) {
  return ((*(this->grid))[y][x] == nullptr);
}

bool Simulator::isRabbitInNextGrid(std::size_t x, std::size_t y) {
  Entity *cell = (*(this->next_grid))[y][x];
  return (cell != nullptr && cell->is_alive &&
          cell->type == EntityType::RABBIT);
}

bool Simulator::isCellEmptyForFox(std::size_t x, std::size_t y) {
  // 1. En next_grid no debe haber entidad (ni roca ni conejo tras su fase de movimiento)
  if ((*(this->next_grid))[y][x] != nullptr) {
    return false;
  }
  // 2. En grid no debe haber habido un zorro al inicio del turno (movimiento simultáneo de zorros)
  Entity *curr = (*(this->grid))[y][x];
  if (curr != nullptr && curr->type == EntityType::FOX) {
    return false;
  }
  return true;
}

bool Simulator::isMoveable(std::size_t x, std::size_t y) {
  Entity *cell = (*(this->grid))[y][x];
  return (cell == nullptr || cell->type != EntityType::ROCK);
}

bool Simulator::isRabbit(std::size_t x, std::size_t y) {
  Entity *cell = (*(this->grid))[y][x];
  return (cell != nullptr && cell->type == EntityType::RABBIT);
}

bool Simulator::isFox(std::size_t x, std::size_t y) {
  Entity *cell = (*(this->grid))[y][x];
  return (cell != nullptr && cell->type == EntityType::FOX);
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getPossibleMovesRabbit(std::size_t x, std::size_t y) {
  size_t h = (*(this->grid)).getHeight();
  size_t w = (*(this->grid)).getWidth();

  std::vector<std::pair<std::size_t, std::size_t>> possible_moves;
  // Clockwise order: North (0), East (1), South (2), West (3)
  if (y > 0 && isEmpty(x, y - 1))
    possible_moves.push_back(std::make_pair(x, y - 1));
  if (x < w - 1 && isEmpty(x + 1, y))
    possible_moves.push_back(std::make_pair(x + 1, y));
  if (y < h - 1 && isEmpty(x, y + 1))
    possible_moves.push_back(std::make_pair(x, y + 1));
  if (x > 0 && isEmpty(x - 1, y))
    possible_moves.push_back(std::make_pair(x - 1, y));

  return possible_moves;
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getRabbitMovesForFox(std::size_t x, std::size_t y) {
  size_t h = (*(this->next_grid)).getHeight();
  size_t w = (*(this->next_grid)).getWidth();

  std::vector<std::pair<std::size_t, std::size_t>> possible_moves;
  // Clockwise order: North (0), East (1), South (2), West (3)
  if (y > 0 && isRabbitInNextGrid(x, y - 1))
    possible_moves.push_back(std::make_pair(x, y - 1));
  if (x < w - 1 && isRabbitInNextGrid(x + 1, y))
    possible_moves.push_back(std::make_pair(x + 1, y));
  if (y < h - 1 && isRabbitInNextGrid(x, y + 1))
    possible_moves.push_back(std::make_pair(x, y + 1));
  if (x > 0 && isRabbitInNextGrid(x - 1, y))
    possible_moves.push_back(std::make_pair(x - 1, y));

  return possible_moves;
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getEmptyMovesForFox(std::size_t x, std::size_t y) {
  size_t h = (*(this->grid)).getHeight();
  size_t w = (*(this->grid)).getWidth();

  std::vector<std::pair<std::size_t, std::size_t>> possible_moves;
  // Clockwise order: North (0), East (1), South (2), West (3)
  if (y > 0 && isCellEmptyForFox(x, y - 1))
    possible_moves.push_back(std::make_pair(x, y - 1));
  if (x < w - 1 && isCellEmptyForFox(x + 1, y))
    possible_moves.push_back(std::make_pair(x + 1, y));
  if (y < h - 1 && isCellEmptyForFox(x, y + 1))
    possible_moves.push_back(std::make_pair(x, y + 1));
  if (x > 0 && isCellEmptyForFox(x - 1, y))
    possible_moves.push_back(std::make_pair(x - 1, y));

  return possible_moves;
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getPossibleMovesFox(std::size_t x, std::size_t y) {
  auto rabbits = getRabbitMovesForFox(x, y);
  if (!rabbits.empty()) {
    return rabbits;
  }
  return getEmptyMovesForFox(x, y);
}

std::vector<std::pair<std::size_t, std::size_t>>
Simulator::getPossibleMoves(std::size_t x, std::size_t y) {
  Entity *cell = (*(this->grid))[y][x];
  if (cell == nullptr) {
    return std::vector<std::pair<std::size_t, std::size_t>>();
  }

  if (cell->type == EntityType::RABBIT) {
    return this->getPossibleMovesRabbit(x, y);
  } else if (cell->type == EntityType::FOX) {
    return this->getPossibleMovesFox(x, y);
  }
  return std::vector<std::pair<std::size_t, std::size_t>>();
}

std::pair<std::size_t, std::size_t> Simulator::selectDisplacement(
    std::uint32_t g, std::size_t x, std::size_t y,
    const std::vector<std::pair<std::size_t, std::size_t>> &possible_moves) {
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

  return this->selectDisplacement(this->step, x, y, possible_moves);
}

// Update at each simulation step
// SECUENTIAL VERSION

void Simulator::update() {
  size_t h = this->grid->getHeight();
  size_t w = this->grid->getWidth();

  // 1. Colocar las rocas en next_grid
  for (auto &rock : this->rocks) {
    this->next_grid->addEntity(rock, std::make_pair(rock->x, rock->y));
  }

  // 2. Mover los conejos
  std::vector<Entity *> move_rabbits;
  std::vector<std::pair<std::size_t, std::size_t>> rabbit_prev_positions;
  std::vector<std::pair<std::size_t, std::size_t>> rabbit_new_positions;
  for (auto &rabbit : this->rabbits) {
    if (!rabbit->is_alive) {
      continue;
    }

    size_t prev_x = rabbit->x;
    size_t prev_y = rabbit->y;

    std::vector<std::pair<std::size_t, std::size_t>> possible_moves =
        getPossibleMovesRabbit(rabbit->x, rabbit->y);

    std::pair<std::size_t, std::size_t> new_pos;
    if (possible_moves.empty()) {
      new_pos = std::make_pair(prev_x, prev_y);
    } else {
      new_pos =
          selectDisplacement(this->step, rabbit->x, rabbit->y, possible_moves);
    }
    move_rabbits.push_back(rabbit);
    rabbit_prev_positions.push_back(std::make_pair(prev_x, prev_y));
    rabbit_new_positions.push_back(new_pos);
  }

  // 2.5. Procesar conflictos entre conejos (solo arbitraje, sin modificar proc_age)
  for (size_t i = 0; i < move_rabbits.size(); i++) {
    Entity *rabbit = move_rabbits[i];
    std::pair<std::size_t, std::size_t> new_pos = rabbit_new_positions[i];

    Entity *occupant = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (occupant == nullptr) {
      this->next_grid->addEntity(rabbit, new_pos);
    } else if (occupant->type == EntityType::RABBIT) {
      // Conflicto de conejos: el de mayor proc_age sobrevive
      if (rabbit->proc_age > occupant->proc_age) {
        occupant->is_alive = false;
        count_rabbit--;
        this->next_grid->addEntity(rabbit, new_pos);
      } else {
        rabbit->is_alive = false;
        count_rabbit--;
      }
    }
  }

  // 2.6. Envejecimiento y procreación de conejos (incluye procreación si se movieron aunque mueran en conflicto)
  std::vector<Entity *> new_rabbits;
  for (size_t i = 0; i < move_rabbits.size(); i++) {
    Entity *rabbit = move_rabbits[i];

    size_t prev_x = rabbit_prev_positions[i].first;
    size_t prev_y = rabbit_prev_positions[i].second;
    size_t new_x = rabbit_new_positions[i].first;
    size_t new_y = rabbit_new_positions[i].second;

    bool didMove = (prev_x != new_x || prev_y != new_y);
    if (rabbit->proc_age >= GEN_PROC_RABBITS && didMove) {
      rabbit->proc_age = 0;

      std::unique_ptr<Entity> child = std::make_unique<Entity>();
      child->type = EntityType::RABBIT;
      child->id = this->entities.size();
      child->x = prev_x;
      child->y = prev_y;
      child->proc_age = 0;
      child->starve_age = 0;
      child->is_alive = true;
      child->just_ate = false;
      this->entities.push_back(std::move(child));
      new_rabbits.push_back(this->entities.back().get());
      this->count_rabbit++;
      this->next_grid->addEntity(this->entities.back().get(),
                                 std::make_pair(prev_x, prev_y));
    } else if (rabbit->is_alive) {
      rabbit->proc_age++;
    }

    if (rabbit->is_alive) {
      rabbit->x = new_x;
      rabbit->y = new_y;
    }
  }

  this->rabbits.insert(this->rabbits.end(), new_rabbits.begin(),
                       new_rabbits.end());

  // 3. Mover los zorros
  std::vector<Entity *> move_foxes;
  std::vector<std::pair<std::size_t, std::size_t>> fox_prev_positions;
  std::vector<std::pair<std::size_t, std::size_t>> fox_new_positions;
  for (auto &fox : this->foxes) {
    if (!fox->is_alive) {
      continue;
    }
    size_t prev_x = fox->x;
    size_t prev_y = fox->y;
    fox->just_ate = false;

    // A. Buscar conejos adyacentes en next_grid
    std::vector<std::pair<std::size_t, std::size_t>> rabbit_moves =
        getRabbitMovesForFox(fox->x, fox->y);

    std::pair<std::size_t, std::size_t> new_pos;

    if (!rabbit_moves.empty()) {
      new_pos = selectDisplacement(this->step, fox->x, fox->y, rabbit_moves);
    } else {
      // Regla de inanición: al no encontrar conejo para comer en este turno, si
      // alcanza GEN_FOOD_FOXES muere antes de buscar vacías
      if (fox->starve_age + 1 >= GEN_FOOD_FOXES) {
        fox->is_alive = false;
        count_fox--;
        fox->starve_age++;
        continue;
      }

      std::vector<std::pair<std::size_t, std::size_t>> empty_moves =
          getEmptyMovesForFox(fox->x, fox->y);

      if (empty_moves.empty()) {
        new_pos = std::make_pair(prev_x, prev_y);
      } else {
        new_pos = selectDisplacement(this->step, fox->x, fox->y, empty_moves);
      }
    }

    move_foxes.push_back(fox);
    fox_prev_positions.push_back(std::make_pair(prev_x, prev_y));
    fox_new_positions.push_back(new_pos);
  }

  // 3.5. Resolver conflictos entre zorros (solo arbitraje, sin modificar starve_age ni proc_age)
  for (size_t i = 0; i < move_foxes.size(); i++) {
    Entity *fox = move_foxes[i];
    std::pair<std::size_t, std::size_t> new_pos = fox_new_positions[i];

    Entity *occupant = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (occupant == nullptr) {
      this->next_grid->addEntity(fox, new_pos);
      fox->just_ate = false;
    } else if (occupant->type == EntityType::RABBIT) {
      occupant->is_alive = false;
      count_rabbit--;
      this->next_grid->addEntity(fox, new_pos);
      fox->just_ate = true;
    } else if (occupant->type == EntityType::FOX) {
      // Conflicto de zorros: mayor proc_age, o menor starve_age si empatan
      bool fox_wins = (fox->proc_age > occupant->proc_age) ||
                      (fox->proc_age == occupant->proc_age &&
                       fox->starve_age < occupant->starve_age);

      if (fox_wins) {
        occupant->is_alive = false;
        count_fox--;
        if (occupant->just_ate) {
          fox->just_ate = true;
        }
        this->next_grid->addEntity(fox, new_pos);
      } else {
        fox->is_alive = false;
        count_fox--;
      }
    }
  }

  // 3.6. Hambre, procreación y envejecimiento de zorros (incluye procreación si se movieron aunque mueran en conflicto)
  std::vector<Entity *> new_foxes;
  for (size_t i = 0; i < move_foxes.size(); i++) {
    Entity *fox = move_foxes[i];

    size_t prev_x = fox_prev_positions[i].first;
    size_t prev_y = fox_prev_positions[i].second;
    size_t new_x = fox_new_positions[i].first;
    size_t new_y = fox_new_positions[i].second;

    bool didMove = (prev_x != new_x || prev_y != new_y);
    if (fox->proc_age >= GEN_PROC_FOXES && didMove) {
      fox->proc_age = 0;

      std::unique_ptr<Entity> child = std::make_unique<Entity>();
      child->type = EntityType::FOX;
      child->id = this->entities.size();
      child->x = prev_x;
      child->y = prev_y;
      child->proc_age = 0;
      child->starve_age = 0;
      child->is_alive = true;
      child->just_ate = false;
      this->entities.push_back(std::move(child));
      new_foxes.push_back(this->entities.back().get());
      this->count_fox++;
      this->next_grid->addEntity(this->entities.back().get(),
                                 std::make_pair(prev_x, prev_y));
    } else if (fox->is_alive) {
      fox->proc_age++;
    }

    if (fox->is_alive) {
      fox->x = new_x;
      fox->y = new_y;

      if (fox->just_ate) {
        fox->starve_age = 0;
      } else {
        fox->starve_age++;
      }
    }
  }

  this->foxes.insert(this->foxes.end(), new_foxes.begin(), new_foxes.end());

  delete this->grid;
  this->grid = this->next_grid;
  this->next_grid = new Grid(w, h);
  this->step++;
}
