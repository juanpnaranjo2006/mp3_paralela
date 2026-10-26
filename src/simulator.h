#ifndef SIMULATOR_H
#define SIMULATOR_H
#include "grid.h"

class Simulator {
private:
  Grid *grid;
  Grid *next_grid;
  std::uint32_t step{0}; // "step = generation"
  std::uint32_t GEN_PROC_RABBITS;
  std::uint32_t GEN_PROC_FOXES;
  std::uint32_t GEN_FOOD_FOXES;

  std::vector<std::unique_ptr<Entity>> entities;
  std::vector<Entity *> rocks;
  std::vector<Entity *> rabbits;
  std::vector<Entity *> foxes;

  // Statistics
  std::uint64_t count_rock{0};
  std::uint64_t count_rabbit{0};
  std::uint64_t count_fox{0};

  // Auxiliary choosing adjacent cell methods

  std::pair<std::size_t, std::size_t> chooseAdjCell(std::size_t x,
                                                    std::size_t y);

  std::pair<int, int> selectDisplacement(
      std::uint32_t g, std::size_t x, std::size_t y,
      std::vector<std::pair<std::size_t, std::size_t>> &possible_moves);

  std::vector<std::pair<std::size_t, std::size_t>>
  getPossibleMoves(std::size_t x, std::size_t y);

  std::vector<std::pair<std::size_t, std::size_t>>
  getPossibleMovesRabbit(std::size_t x, std::size_t y);

  std::vector<std::pair<std::size_t, std::size_t>>
  getPossibleMovesFox(std::size_t x, std::size_t y);

  bool isMoveable(std::size_t x, std::size_t y);

  bool isRabbit(std::size_t x, std::size_t y);

  bool isFox(std::size_t x, std::size_t y);

public:
  Simulator() = default;
  Simulator(std::size_t width, std::size_t height,
            std::vector<std::unique_ptr<Entity>> &entities,
            std::vector<std::pair<std::size_t, std::size_t>> &positions);

  ~Simulator() = default;

  Simulator(const Simulator &) = delete;
  Simulator &operator=(const Simulator &) = delete;

  Simulator(Simulator &&) noexcept = default;
  Simulator &operator=(Simulator &&) noexcept = default;

  // Getters

  Grid *getGrid() { return this->grid; }

  std::uint64_t getRockCount() const { return this->count_rock; }
  std::uint64_t getRabbitCount() const { return this->count_rabbit; }
  std::uint64_t getFoxCount() const { return this->count_fox; }
  std::uint32_t getStep() const { return this->step; }

  void update();
};

#endif