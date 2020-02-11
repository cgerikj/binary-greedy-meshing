#ifndef NOISE_H
#define NOISE_H

#include <vector>
#include <glm/glm.hpp>
#include "../libraries/FastNoise/FastNoise.h"
#include "utility.h"

class Noise {
public:
  Noise() {
    noise.SetFrequency(0.02);
    noise.SetFractalType(FastNoise::FBM);
    noise.SetFractalOctaves(5);
    noise.SetFractalLacunarity(2.0);
  }

  void generateTerrain(std::vector<uint8_t>& voxels, int seed) {
    noise.SetSeed(seed);

    for (int x = 0; x < CS_P; x++) {
      for (int y = CS_P-1; y--;) {
        for (int z = 0; z < CS_P; z++) {
          float val = ((noise.GetSimplexFractal(x, y, z)) + 1.0f) / 2.0f;

          if (val > glm::smoothstep(0.15f, 1.0f, (float)y / (float)CS_P)) {
            int i = get_yzx_index(x, y, z);
            int i_above = get_yzx_index(x, y + 1, z);

            switch (voxels.at(i_above)) {
              case 0:
                voxels.at(i) = 2;
                break;
              default:
                voxels.at(i) = 1;
                break;
            }
          }
        }
      }
    }
  }

  void generateWhiteNoiseTerrain(std::vector<uint8_t>& voxels, int seed) {
    whiteNoise.SetSeed(seed);

    for (int x = 1; x < CS_P; x++) {
      for (int y = CS_P; y--;) {
        for (int z = 1; z < CS_P; z++) {
          float noise = (whiteNoise.GetWhiteNoise(x, y, z));
          int i = get_yzx_index(x, y, z);
          if (noise > 0.8f) {
            voxels.at(i) = 1;
          }
          else if (noise > 0.6f) {
            voxels.at(i) = 2;
          }
          else if (noise > 0.5f) {
            voxels.at(i) = 3;
          }
        }
      }
    }
  }

  FastNoise noise;
  FastNoise whiteNoise;
};

#endif