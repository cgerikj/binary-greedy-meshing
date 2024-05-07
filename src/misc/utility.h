#ifndef UTILITY_H
#define UTILITY_H

#include <glm/glm.hpp>
#include "../mesher.h"

inline int get_yzx_index(int x, int y, int z) {
  return z + (x * CS_P) + (y * CS_P2);
}

#endif