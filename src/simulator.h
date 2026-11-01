#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "grid.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <pthread.h>
#include <utility>
#include <vector>

// Barrera de sincronización reusable implementada con variables de condición POSIX
struct PthreadBarrier {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  std::size_t count{0};
  std::size_t total{0};
  std::size_t cycle{0};

  void init(std::size_t n) {
    pthread_mutex_init(&this->mutex, nullptr);
    pthread_cond_init(&this->cond, nullptr);
    this->count = 0;
    this->total = n;
    this->cycle = 0;
  }

  void wait() {
    pthread_mutex_lock(&this->mutex);
    std::size_t c = this->cycle;
    this->count++;
    if (this->count == this->total) {
      this->cycle++;
      this->count = 0;
      pthread_cond_broadcast(&this->cond);
    } else {
      while (c == this->cycle) {
        pthread_cond_wait(&this->cond, &this->mutex);
      }
    }
    pthread_mutex_unlock(&this->mutex);
  }

  void destroy() {
    pthread_mutex_destroy(&this->mutex);
    pthread_cond_destroy(&this->cond);
  }
};

class Simulator;

// Contexto local para cada hilo de trabajo fijo
struct WorkerContext {
  std::size_t thread_id{0};
  Simulator *sim{nullptr};

  // Contadores locales de bajas para evitar contención
  std::uint64_t dead_rabbits{0};
  std::uint64_t eaten_rabbits{0};
  std::uint64_t dead_foxes{0};

  // Almacenamiento intermedio de movimientos calculados
  std::vector<Entity *> planned_rabbits;
  std::vector<std::pair<std::size_t, std::size_t>> rabbit_prev_pos;
  std::vector<std::pair<std::size_t, std::size_t>> rabbit_new_pos;

  std::vector<Entity *> planned_foxes;
  std::vector<std::pair<std::size_t, std::size_t>> fox_prev_pos;
  std::vector<std::pair<std::size_t, std::size_t>> fox_new_pos;

  // Nuevas entidades nacidas durante la generación en este hilo
  std::vector<std::unique_ptr<Entity>> local_new_entities;
  std::vector<Entity *> local_new_rabbits;
  std::vector<Entity *> local_new_foxes;

  void reset() {
    dead_rabbits = 0;
    eaten_rabbits = 0;
    dead_foxes = 0;
    planned_rabbits.clear();
    rabbit_prev_pos.clear();
    rabbit_new_pos.clear();
    planned_foxes.clear();
    fox_prev_pos.clear();
    fox_new_pos.clear();
    local_new_entities.clear();
    local_new_rabbits.clear();
    local_new_foxes.clear();
  }
};

class Simulator {
public:
  // Número fijo de hilos según requerimiento
  static constexpr std::size_t NUM_THREADS = 4;

private:
  Grid *grid{nullptr};
  Grid *next_grid{nullptr};
  std::uint32_t step{0}; // "step = generation"
  std::uint32_t GEN_PROC_RABBITS{0};
  std::uint32_t GEN_PROC_FOXES{0};
  std::uint32_t GEN_FOOD_FOXES{0};

  std::vector<std::unique_ptr<Entity>> entities;
  std::vector<Entity *> rocks;
  std::vector<Entity *> rabbits;
  std::vector<Entity *> foxes;

  // Estadísticas globales
  std::uint64_t count_rock{0};
  std::uint64_t count_rabbit{0};
  std::uint64_t count_fox{0};

  // Concurrencia Pthreads
  pthread_t *workers{nullptr};
  WorkerContext *worker_contexts{nullptr};
  PthreadBarrier start_barrier;
  PthreadBarrier step_barrier;
  PthreadBarrier done_barrier;
  pthread_mutex_t *row_mutexes{nullptr};
  bool stop_simulation{false};

  void initThreadPool(std::size_t height);
  void cleanupThreadPool();
  static void *workerRoutine(void *arg);

  // Fases concurrentes ejecutadas por los hilos de trabajo
  void computeRabbitMoves(WorkerContext *ctx);
  void resolveRabbitConflicts(WorkerContext *ctx);
  void rabbitAgingAndProcreation(WorkerContext *ctx);
  void computeFoxMoves(WorkerContext *ctx);
  void resolveFoxConflicts(WorkerContext *ctx);
  void foxAgingAndProcreation(WorkerContext *ctx);

  // Métodos auxiliares de selección de celdas adyacentes
  std::pair<std::size_t, std::size_t> chooseAdjCell(std::size_t x,
                                                    std::size_t y);

  std::pair<std::size_t, std::size_t> selectDisplacement(
      std::uint32_t g, std::size_t x, std::size_t y,
      const std::vector<std::pair<std::size_t, std::size_t>> &possible_moves);

  std::vector<std::pair<std::size_t, std::size_t>>
  getPossibleMoves(std::size_t x, std::size_t y);

  std::vector<std::pair<std::size_t, std::size_t>>
  getPossibleMovesRabbit(std::size_t x, std::size_t y);

  std::vector<std::pair<std::size_t, std::size_t>>
  getRabbitMovesForFox(std::size_t x, std::size_t y);

  std::vector<std::pair<std::size_t, std::size_t>>
  getEmptyMovesForFox(std::size_t x, std::size_t y);

  std::vector<std::pair<std::size_t, std::size_t>>
  getPossibleMovesFox(std::size_t x, std::size_t y);

  bool isEmpty(std::size_t x, std::size_t y);

  bool isRabbitInNextGrid(std::size_t x, std::size_t y);

  bool isCellEmptyForFox(std::size_t x, std::size_t y);

  bool isMoveable(std::size_t x, std::size_t y);

  bool isRabbit(std::size_t x, std::size_t y);

  bool isFox(std::size_t x, std::size_t y);

public:
  Simulator() = default;
  Simulator(std::size_t width, std::size_t height,
            std::vector<std::unique_ptr<Entity>> &entities,
            std::vector<std::pair<std::size_t, std::size_t>> &positions);

  Simulator(std::size_t width, std::size_t height,
            std::uint32_t gen_proc_rabbits,
            std::uint32_t gen_proc_foxes,
            std::uint32_t gen_food_foxes,
            std::vector<std::unique_ptr<Entity>> &entities,
            std::vector<std::pair<std::size_t, std::size_t>> &positions);

  ~Simulator();

  Simulator(const Simulator &) = delete;
  Simulator &operator=(const Simulator &) = delete;

  Simulator(Simulator &&) = delete;
  Simulator &operator=(Simulator &&) = delete;

  // Getters
  Grid *getGrid() { return this->grid; }

  std::uint64_t getRockCount() const { return this->count_rock; }
  std::uint64_t getRabbitCount() const { return this->count_rabbit; }
  std::uint64_t getFoxCount() const { return this->count_fox; }
  std::uint32_t getStep() const { return this->step; }

  std::uint32_t getGenProcRabbits() const { return this->GEN_PROC_RABBITS; }
  std::uint32_t getGenProcFoxes() const { return this->GEN_PROC_FOXES; }
  std::uint32_t getGenFoodFoxes() const { return this->GEN_FOOD_FOXES; }

  void setGenProcRabbits(std::uint32_t v) { this->GEN_PROC_RABBITS = v; }
  void setGenProcFoxes(std::uint32_t v) { this->GEN_PROC_FOXES = v; }
  void setGenFoodFoxes(std::uint32_t v) { this->GEN_FOOD_FOXES = v; }

  // Método paralelizado con Pthreads
  void update();
};

#endif
