#ifndef UTILITY_H
#define UTILITY_H

#include <glm/glm.hpp>
#include "../mesher.h"

inline int get_yzx_index(int x, int y, int z) {
  return z + (x * CS_P) + (y * CS_P2);
}

uint32_t get_xyz_key(int8_t x, int8_t y, int8_t z) {
  return (((uint32_t)z << 16) | ((uint32_t)y << 8) | (uint32_t)x);
}

  glm::ivec3 parse_xyz_key(uint32_t key) {
  return glm::ivec3(
    (key & 255),
    (key >> 8 & 255),
    (key >> 16 & 255)
  );
}

#endif