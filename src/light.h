#ifndef LIGHT_H
#define LIGHT_H

#include <vector>
#include <stdint.h>
#include "constants.h"

void calculate_light(std::vector<uint8_t>& voxels, std::vector<uint8_t>& light_map) {
    for (int y = CS_P; y--;) {
      for (int zx = 0; zx < CS_P2; zx++) {
        int i = (y * CS_P2) + zx;
        if (y == CS_P - 1) {
          light_map.at(i) = 15;
        }
        else {
          auto light_above = light_map.at(i + CS_P2);
          auto voxel = voxels.at(i);
          if (voxels.at(i) == 0 && light_above > 0) {
            light_map.at(i) = light_above;
          }
          else {
            light_map.at(i) = 4;
          }
        }
      }
    }
  }

#endif