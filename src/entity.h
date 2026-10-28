#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>

enum class EntityType : std::uint8_t { ROCK, RABBIT, FOX };

struct Entity {
  EntityType type;
  std::uint32_t id;
  std::uint32_t proc_age;
  std::uint32_t starve_age;
  bool is_alive;
  bool just_ate;
  std::size_t x;
  std::size_t y;
};

#endif