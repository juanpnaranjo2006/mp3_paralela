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
  /*
  BUG FIX: Previamente cuando ya había un fox en next_grid, esta
  casilla no se consideraba como opción.

  En realidad el enunciado
  pedía actuar como si todos los zorros se movieran al tiempo y
  dejar en cada casilla al zorro con más chances de sobrevivir
  de acuerdo la regla definida.

  La anterior versión era un problema ya que impedía mover a
  un zorro moverse a una casilla en next_grid si ya había otro zorro en ella,
  siendo que este no necesiramente el primer zorro en llegar a la casilla
  es el que debe quedarse en ella.

  La solución es permitir el movimiento a celdas ya ocupadas en next_grid
  y dejar que el criterio de desempate deje vivo al zorro más óptimo.
  */
  Entity *curr = (*(this->grid))[y][x];
  return (curr == nullptr);
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

  size_t prev_x;
  size_t prev_y;

  // 2. Mover los conejos
  std::vector<Entity *> new_rabbits;
  for (auto &rabbit : this->rabbits) {
    if (!rabbit->is_alive) {
      continue;
    }
    prev_x = rabbit->x;
    prev_y = rabbit->y;

    std::vector<std::pair<std::size_t, std::size_t>> possible_moves =
        getPossibleMovesRabbit(rabbit->x, rabbit->y);

    std::pair<std::size_t, std::size_t> new_pos;
    if (possible_moves.empty()) {
      new_pos = std::make_pair(prev_x, prev_y);
    } else {
      new_pos =
          selectDisplacement(this->step, rabbit->x, rabbit->y, possible_moves);
    }

    if (new_pos.first == prev_x && new_pos.second == prev_y) {
      this->next_grid->addEntity(rabbit, new_pos);
      rabbit->proc_age++;
      continue;
    }

    Entity *neighbor = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (neighbor == nullptr) {
      this->next_grid->addEntity(rabbit, new_pos);
      rabbit->x = new_pos.first;
      rabbit->y = new_pos.second;
    } else if (neighbor->type == EntityType::RABBIT) {
      // Conflicto de conejos: el de mayor proc_age sobrevive
      if (rabbit->proc_age > neighbor->proc_age) {
        neighbor->is_alive = false;
        count_rabbit--;
        this->next_grid->addEntity(rabbit, new_pos);
        rabbit->x = new_pos.first;
        rabbit->y = new_pos.second;
      } else {
        rabbit->is_alive = false;
        count_rabbit--;
      }
    }

    // Procreación si sobrevivió y se movió
    if (rabbit->is_alive) {
      if (rabbit->proc_age >= GEN_PROC_RABBITS) {
        rabbit->proc_age = 0;

        std::unique_ptr<Entity> child = std::make_unique<Entity>();
        child->type = EntityType::RABBIT;
        child->id = this->entities.size();
        child->x = prev_x;
        child->y = prev_y;
        child->proc_age = 0;
        child->is_alive = true;
        this->entities.push_back(std::move(child));
        new_rabbits.push_back(this->entities.back().get());
        this->count_rabbit++;
        this->next_grid->addEntity(this->entities.back().get(),
                                   std::make_pair(prev_x, prev_y));
      } else {
        rabbit->proc_age++;
      }
    }
  }
  this->rabbits.insert(this->rabbits.end(), new_rabbits.begin(),
                       new_rabbits.end());

  // 3. Mover los zorros
  std::vector<Entity *> new_foxes;
  std::vector<Entity *> dead_rabbits;
  for (auto &fox : this->foxes) {
    if (!fox->is_alive) {
      continue;
    }
    prev_x = fox->x;
    prev_y = fox->y;

    // A. Buscar conejos adyacentes en next_grid
    std::vector<std::pair<std::size_t, std::size_t>> rabbit_moves =
        getRabbitMovesForFox(fox->x, fox->y);

    std::pair<std::size_t, std::size_t> new_pos;
    bool hunting = false;

    if (!rabbit_moves.empty()) {
      hunting = true;
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

    if (new_pos.first == prev_x && new_pos.second == prev_y) {
      this->next_grid->addEntity(fox, new_pos);
      fox->starve_age++;
      fox->proc_age++;
      continue;
    }

    Entity *neighbor = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (neighbor == nullptr) {
      this->next_grid->addEntity(fox, new_pos);
      fox->x = new_pos.first;
      fox->y = new_pos.second;
      fox->starve_age++;
    } else if (neighbor->type == EntityType::RABBIT) {
      // BUG: Matar al conejo hace que al procesar otros zorros no lo consideren
      // como posible presa cuando se supone que todos los zorros se mueven a la
      // vez y gana el más apto.

      // neighbor->is_alive = false;
      // count_rabbit--;

      /*Solución:
      TODO: Solución a este problema
      Propuesta (también simplificaria conejos):
      No actualizar de una el grid sino guardar en una pila todos
      los movimientos de los animales de la misma especie.
      Luego, se procesa la pila y los conflictos que vayan surgiendo
      se resuelven, como escoger al animal apto para sobrevivir
      según el criterio definido.
      */
      dead_rabbits.push_back(neighbor);

      this->next_grid->addEntity(fox, new_pos);
      fox->x = new_pos.first;
      fox->y = new_pos.second;
      fox->starve_age = 0;
    } else if (neighbor->type == EntityType::FOX) {
      // Conflicto de zorros: mayor proc_age, o menor starve_age si empatan
      bool fox_wins = (fox->proc_age > neighbor->proc_age) ||
                      (fox->proc_age == neighbor->proc_age &&
                       fox->starve_age < neighbor->starve_age);

      bool ate_rabbit = hunting || (neighbor->starve_age == 0);

      if (fox_wins) {
        neighbor->is_alive = false;
        count_fox--;

        this->next_grid->addEntity(fox, new_pos);
        fox->x = new_pos.first;
        fox->y = new_pos.second;
        if (ate_rabbit) {
          fox->starve_age = 0;
        } else {
          fox->starve_age++;
        }
      } else {
        fox->is_alive = false;
        count_fox--;
      }
    }

    // Procreación si sobrevivió y se movió
    if (fox->is_alive) {
      if (fox->proc_age >= GEN_PROC_FOXES) {
        fox->proc_age = 0;

        std::unique_ptr<Entity> child = std::make_unique<Entity>();
        child->type = EntityType::FOX;
        child->id = this->entities.size();
        child->x = prev_x;
        child->y = prev_y;
        child->proc_age = 0;
        child->starve_age = 0;
        child->is_alive = true;
        child->is_new = false;
        this->entities.push_back(std::move(child));
        new_foxes.push_back(this->entities.back().get());
        this->count_fox++;
        this->next_grid->addEntity(this->entities.back().get(),
                                   std::make_pair(prev_x, prev_y));
      } else {
        fox->proc_age++;
      }
    }
  }
  this->foxes.insert(this->foxes.end(), new_foxes.begin(), new_foxes.end());

  // Matar conejos cazados
  for (auto &rabbit : dead_rabbits) {
    rabbit->is_alive = false;
    count_rabbit--;
  }

  delete this->grid;
  this->grid = this->next_grid;
  this->next_grid = new Grid(w, h);
  this->step++;
}
