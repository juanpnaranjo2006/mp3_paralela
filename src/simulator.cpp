#include "simulator.h"
#include "entity.h"
#include <algorithm>
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

  this->initThreadPool(height);
}

Simulator::~Simulator() {
  this->cleanupThreadPool();
  delete this->grid;
  delete this->next_grid;
}

void Simulator::initThreadPool(std::size_t height) {
  this->stop_simulation = false;

  this->start_barrier.init(NUM_THREADS + 1);
  this->step_barrier.init(NUM_THREADS);
  this->done_barrier.init(NUM_THREADS + 1);

  this->row_mutexes = new pthread_mutex_t[height];
  for (std::size_t y = 0; y < height; ++y) {
    pthread_mutex_init(&this->row_mutexes[y], nullptr);
  }

  this->workers = new pthread_t[NUM_THREADS];
  this->worker_contexts = new WorkerContext[NUM_THREADS];

  for (std::size_t t = 0; t < NUM_THREADS; ++t) {
    this->worker_contexts[t].thread_id = t;
    this->worker_contexts[t].sim = this;
    pthread_create(&this->workers[t], nullptr, Simulator::workerRoutine,
                   &this->worker_contexts[t]);
  }
}

void Simulator::cleanupThreadPool() {
  if (this->workers != nullptr) {
    this->stop_simulation = true;
    this->start_barrier.wait();

    for (std::size_t t = 0; t < NUM_THREADS; ++t) {
      pthread_join(this->workers[t], nullptr);
    }
    delete[] this->workers;
    this->workers = nullptr;

    delete[] this->worker_contexts;
    this->worker_contexts = nullptr;

    this->start_barrier.destroy();
    this->step_barrier.destroy();
    this->done_barrier.destroy();
  }

  if (this->row_mutexes != nullptr) {
    std::size_t h = this->grid != nullptr ? this->grid->getHeight() : 0;
    for (std::size_t y = 0; y < h; ++y) {
      pthread_mutex_destroy(&this->row_mutexes[y]);
    }
    delete[] this->row_mutexes;
    this->row_mutexes = nullptr;
  }
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
  auto rabbits_adj = getRabbitMovesForFox(x, y);
  if (!rabbits_adj.empty()) {
    return rabbits_adj;
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

// -------------------------------------------------------------
// Fases Concurrentes ejecutadas por los hilos de trabajo
// -------------------------------------------------------------

void Simulator::computeRabbitMoves(WorkerContext *ctx) {
  std::size_t total = this->rabbits.size();
  std::size_t start = (ctx->thread_id * total) / NUM_THREADS;
  std::size_t end = ((ctx->thread_id + 1) * total) / NUM_THREADS;

  for (std::size_t i = start; i < end; ++i) {
    Entity *rabbit = this->rabbits[i];
    if (!rabbit->is_alive) {
      continue;
    }

    std::size_t prev_x = rabbit->x;
    std::size_t prev_y = rabbit->y;

    auto possible_moves = this->getPossibleMovesRabbit(prev_x, prev_y);

    std::pair<std::size_t, std::size_t> new_pos;
    if (possible_moves.empty()) {
      new_pos = std::make_pair(prev_x, prev_y);
    } else {
      new_pos = this->selectDisplacement(this->step, prev_x, prev_y, possible_moves);
    }

    ctx->planned_rabbits.push_back(rabbit);
    ctx->rabbit_prev_pos.push_back(std::make_pair(prev_x, prev_y));
    ctx->rabbit_new_pos.push_back(new_pos);
  }
}

void Simulator::resolveRabbitConflicts(WorkerContext *ctx) {
  for (std::size_t i = 0; i < ctx->planned_rabbits.size(); ++i) {
    Entity *rabbit = ctx->planned_rabbits[i];
    auto new_pos = ctx->rabbit_new_pos[i];
    std::size_t row = new_pos.second;

    pthread_mutex_lock(&this->row_mutexes[row]);

    Entity *occupant = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (occupant == nullptr) {
      this->next_grid->addEntity(rabbit, new_pos);
    } else if (occupant->type == EntityType::RABBIT) {
      // Conflicto de conejos: el de mayor proc_age sobrevive.
      // Desempate determinista por ID si empatan en proc_age.
      bool rabbit_wins = (rabbit->proc_age > occupant->proc_age) ||
                         (rabbit->proc_age == occupant->proc_age && rabbit->id < occupant->id);
      if (rabbit_wins) {
        occupant->is_alive = false;
        ctx->dead_rabbits++;
        this->next_grid->addEntity(rabbit, new_pos);
      } else {
        rabbit->is_alive = false;
        ctx->dead_rabbits++;
      }
    }

    pthread_mutex_unlock(&this->row_mutexes[row]);
  }
}

void Simulator::rabbitAgingAndProcreation(WorkerContext *ctx) {
  for (std::size_t i = 0; i < ctx->planned_rabbits.size(); ++i) {
    Entity *rabbit = ctx->planned_rabbits[i];
    std::size_t prev_x = ctx->rabbit_prev_pos[i].first;
    std::size_t prev_y = ctx->rabbit_prev_pos[i].second;
    std::size_t new_x = ctx->rabbit_new_pos[i].first;
    std::size_t new_y = ctx->rabbit_new_pos[i].second;

    bool didMove = (prev_x != new_x || prev_y != new_y);
    if (rabbit->proc_age >= this->GEN_PROC_RABBITS && didMove) {
      rabbit->proc_age = 0;

      auto child = std::make_unique<Entity>();
      child->type = EntityType::RABBIT;
      child->id = 0; // Se asignará ordenadamente al consolidar
      child->x = prev_x;
      child->y = prev_y;
      child->proc_age = 0;
      child->starve_age = 0;
      child->is_alive = true;
      child->just_ate = false;

      pthread_mutex_lock(&this->row_mutexes[prev_y]);
      this->next_grid->addEntity(child.get(), std::make_pair(prev_x, prev_y));
      pthread_mutex_unlock(&this->row_mutexes[prev_y]);

      ctx->local_new_rabbits.push_back(child.get());
      ctx->local_new_entities.push_back(std::move(child));
    } else if (rabbit->is_alive) {
      rabbit->proc_age++;
    }

    if (rabbit->is_alive) {
      rabbit->x = new_x;
      rabbit->y = new_y;
    }
  }
}

void Simulator::computeFoxMoves(WorkerContext *ctx) {
  std::size_t total = this->foxes.size();
  std::size_t start = (ctx->thread_id * total) / NUM_THREADS;
  std::size_t end = ((ctx->thread_id + 1) * total) / NUM_THREADS;

  for (std::size_t i = start; i < end; ++i) {
    Entity *fox = this->foxes[i];
    if (!fox->is_alive) {
      continue;
    }
    std::size_t prev_x = fox->x;
    std::size_t prev_y = fox->y;
    fox->just_ate = false;

    // A. Buscar conejos adyacentes en next_grid
    auto rabbit_moves = this->getRabbitMovesForFox(fox->x, fox->y);
    std::pair<std::size_t, std::size_t> new_pos;

    if (!rabbit_moves.empty()) {
      new_pos = this->selectDisplacement(this->step, fox->x, fox->y, rabbit_moves);
    } else {
      // Regla de inanición
      if (fox->starve_age + 1 >= this->GEN_FOOD_FOXES) {
        fox->is_alive = false;
        ctx->dead_foxes++;
        fox->starve_age++;
        continue;
      }

      auto empty_moves = this->getEmptyMovesForFox(fox->x, fox->y);
      if (empty_moves.empty()) {
        new_pos = std::make_pair(prev_x, prev_y);
      } else {
        new_pos = this->selectDisplacement(this->step, fox->x, fox->y, empty_moves);
      }
    }

    ctx->planned_foxes.push_back(fox);
    ctx->fox_prev_pos.push_back(std::make_pair(prev_x, prev_y));
    ctx->fox_new_pos.push_back(new_pos);
  }
}

void Simulator::resolveFoxConflicts(WorkerContext *ctx) {
  for (std::size_t i = 0; i < ctx->planned_foxes.size(); ++i) {
    Entity *fox = ctx->planned_foxes[i];
    auto new_pos = ctx->fox_new_pos[i];
    std::size_t row = new_pos.second;

    pthread_mutex_lock(&this->row_mutexes[row]);

    Entity *occupant = (*(this->next_grid))[new_pos.second][new_pos.first];
    if (occupant == nullptr) {
      this->next_grid->addEntity(fox, new_pos);
      fox->just_ate = false;
    } else if (occupant->type == EntityType::RABBIT) {
      occupant->is_alive = false;
      ctx->eaten_rabbits++;
      this->next_grid->addEntity(fox, new_pos);
      fox->just_ate = true;
    } else if (occupant->type == EntityType::FOX) {
      // Conflicto de zorros: mayor proc_age, luego menor starve_age.
      // Desempate determinista por ID si empatan en ambos.
      bool fox_wins = (fox->proc_age > occupant->proc_age) ||
                      (fox->proc_age == occupant->proc_age &&
                       fox->starve_age < occupant->starve_age) ||
                      (fox->proc_age == occupant->proc_age &&
                       fox->starve_age == occupant->starve_age &&
                       fox->id < occupant->id);

      if (fox_wins) {
        occupant->is_alive = false;
        ctx->dead_foxes++;
        if (occupant->just_ate) {
          fox->just_ate = true;
        }
        this->next_grid->addEntity(fox, new_pos);
      } else {
        fox->is_alive = false;
        ctx->dead_foxes++;
      }
    }

    pthread_mutex_unlock(&this->row_mutexes[row]);
  }
}

void Simulator::foxAgingAndProcreation(WorkerContext *ctx) {
  for (std::size_t i = 0; i < ctx->planned_foxes.size(); ++i) {
    Entity *fox = ctx->planned_foxes[i];
    std::size_t prev_x = ctx->fox_prev_pos[i].first;
    std::size_t prev_y = ctx->fox_prev_pos[i].second;
    std::size_t new_x = ctx->fox_new_pos[i].first;
    std::size_t new_y = ctx->fox_new_pos[i].second;

    bool didMove = (prev_x != new_x || prev_y != new_y);
    if (fox->proc_age >= this->GEN_PROC_FOXES && didMove) {
      fox->proc_age = 0;

      auto child = std::make_unique<Entity>();
      child->type = EntityType::FOX;
      child->id = 0; // Se asignará ordenadamente al consolidar
      child->x = prev_x;
      child->y = prev_y;
      child->proc_age = 0;
      child->starve_age = 0;
      child->is_alive = true;
      child->just_ate = false;

      pthread_mutex_lock(&this->row_mutexes[prev_y]);
      this->next_grid->addEntity(child.get(), std::make_pair(prev_x, prev_y));
      pthread_mutex_unlock(&this->row_mutexes[prev_y]);

      ctx->local_new_foxes.push_back(child.get());
      ctx->local_new_entities.push_back(std::move(child));
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
}

// -------------------------------------------------------------
// Rutina del Hilo de Trabajo (Worker Thread Loop)
// -------------------------------------------------------------

void *Simulator::workerRoutine(void *arg) {
  WorkerContext *ctx = static_cast<WorkerContext *>(arg);
  Simulator *sim = ctx->sim;

  while (true) {
    // 1. Esperar señal de inicio de generación del hilo principal
    sim->start_barrier.wait();
    if (sim->stop_simulation) {
      break;
    }

    // 2. Fase de conejos: cálculo de movimientos
    sim->computeRabbitMoves(ctx);
    sim->step_barrier.wait();

    // 3. Fase de conejos: arbitraje de conflictos en next_grid
    sim->resolveRabbitConflicts(ctx);
    sim->step_barrier.wait();

    // 4. Fase de conejos: envejecimiento y procreación
    sim->rabbitAgingAndProcreation(ctx);
    sim->step_barrier.wait();

    // 5. Fase de zorros: cálculo de movimientos e inanición
    sim->computeFoxMoves(ctx);
    sim->step_barrier.wait();

    // 6. Fase de zorros: arbitraje y depredación
    sim->resolveFoxConflicts(ctx);
    sim->step_barrier.wait();

    // 7. Fase de zorros: envejecimiento y procreación
    sim->foxAgingAndProcreation(ctx);
    sim->step_barrier.wait();

    // 8. Notificar al hilo principal que este worker ha finalizado su ciclo
    sim->done_barrier.wait();
  }

  return nullptr;
}

// -------------------------------------------------------------
// Update paralelizado con Pthreads (Llamado por el hilo principal)
// -------------------------------------------------------------

void Simulator::update() {
  size_t h = this->grid->getHeight();
  size_t w = this->grid->getWidth();

  // 1. Colocar las rocas en next_grid
  for (auto &rock : this->rocks) {
    this->next_grid->addEntity(rock, std::make_pair(rock->x, rock->y));
  }

  // 2. Limpiar buffers y estadísticas locales de cada hilo
  for (std::size_t t = 0; t < NUM_THREADS; ++t) {
    this->worker_contexts[t].reset();
  }

  // 3. Despertar a los hilos de trabajo para ejecutar las fases concurrentes
  this->start_barrier.wait();

  // 4. Esperar a que todos los hilos completen sus fases
  this->done_barrier.wait();

  // 5. Consolidar entidades recién nacidas y actualizar contadores
  for (std::size_t t = 0; t < NUM_THREADS; ++t) {
    auto &ctx = this->worker_contexts[t];

    this->count_rabbit -= (ctx.dead_rabbits + ctx.eaten_rabbits);
    this->count_rabbit += ctx.local_new_rabbits.size();

    this->count_fox -= ctx.dead_foxes;
    this->count_fox += ctx.local_new_foxes.size();

    for (auto &child : ctx.local_new_entities) {
      child->id = static_cast<std::uint32_t>(this->entities.size());
      this->entities.push_back(std::move(child));
    }
    this->rabbits.insert(this->rabbits.end(), ctx.local_new_rabbits.begin(),
                         ctx.local_new_rabbits.end());
    this->foxes.insert(this->foxes.end(), ctx.local_new_foxes.begin(),
                       ctx.local_new_foxes.end());
  }

  // 6. Intercambiar grillas y avanzar paso de simulación
  delete this->grid;
  this->grid = this->next_grid;
  this->next_grid = new Grid(w, h);
  this->step++;
}
