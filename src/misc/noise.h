#ifndef NOISE_H
#define NOISE_H

#include <vector>
#include <glm/glm.hpp>
#include "../libraries/FastNoise/FastNoise.h"
#include "utility.h"

class Noise {
public:
  Noise() {
    noise1.SetFrequency(0.02);
    noise1.SetFractalType(FastNoise::FBM);
    noise1.SetFractalOctaves(5);
    noise1.SetFractalLacunarity(2.0);

    noise2.SetFrequency(0.006);
    noise2.SetFractalType(FastNoise::FBM);
    noise2.SetFractalOctaves(5);
    noise2.SetFractalLacunarity(2.0);
  }

  void generateTerrainV1(uint8_t* voxels, uint64_t* opaqueMask, int seed, int offsetX, int offsetZ) {
    noise1.SetSeed(seed);

    for (int x = 0; x < CS_P; x++) {
      for (int y = CS_P - 1; y--;) {
        for (int z = 0; z < CS_P; z++) {
          float val = ((noise1.GetSimplexFractal((offsetX * CS) + x, y, (offsetZ * CS) + z)) + 1.0f) / 2.0f;

          if (val > glm::smoothstep(0.15f, 1.0f, (float) y / (float) CS_P)) {
            int i = get_yzx_index(x, y, z);
            int i_above = get_yzx_index(x, y + 1, z);

            opaqueMask[(y * CS_P) + x] |= 1ull << z;

            switch (voxels[i_above]) {
            case 0:
              voxels[i] = 3;
              break;
            default:
              voxels[i] = 2;
              break;
            }
          }
        }
      }
    }
  }

  void generateTerrainV2(uint8_t* voxels, uint64_t* opaqueMask, int offsetX, int offsetZ, int seed) {
    noise2.SetSeed(seed);

    for (int x = 0; x < CS_P; x++) {
      for (int y = CS_P - 1; y--;) {
        for (int z = 0; z < CS_P; z++) {
          float val = (noise2.GetSimplexFractal((offsetX * CS) + x, y, (offsetZ * CS) + z) + 0.6f) / 1.35f;

          int i = get_yzx_index(x, y, z);

          if (val > (float) y / (float) CS_P) {
            int i_above = get_yzx_index(x, y + 1, z);

            opaqueMask[(y * CS_P) + x] |= 1ull << z;

            switch (voxels[i_above]) {
            case 0:
              voxels[i] = 3;
              break;
            default:
              voxels[i] = 2;
              break;
            }
          }
          else if (y < 25) {
            opaqueMask[(y * CS_P) + x] |= 1ull << z;
            voxels[i] = 1;
          }
        }
      }
    }
  }

  void generateWhiteNoiseTerrain(uint8_t* voxels, uint64_t* opaqueMask, int seed) {
    whiteNoise.SetSeed(seed);

    for (int x = 1; x < CS_P; x++) {
      for (int y = CS_P; y--;) {
        for (int z = 1; z < CS_P; z++) {
          float noise = (whiteNoise.GetWhiteNoise(x, y, z));
          int i = get_yzx_index(x, y, z);

          if (noise > 0.5f) opaqueMask[(y * CS_P) + x] |= 1ull << z;

          if (noise > 0.8f) {
            voxels[i] = 1;
          } else if (noise > 0.6f) {
            voxels[i] = 2;
          } else if (noise > 0.5f) {
            voxels[i] = 3;
          }
        }
      }
    }
  }

  FastNoise noise1;
  FastNoise noise2;
  FastNoise whiteNoise;
};

#endif