#ifndef NOISE_H
#define NOISE_H

#include <vector>
#include <glm/glm.hpp>
#include "../libraries/FastNoiseSIMD/include/FastNoiseSIMD.h"


class Noise {
public:
  Noise() {
    noise = FastNoiseSIMD::NewFastNoiseSIMD();
    noise->SetSeed(1337);
    noise->SetNoiseType(FastNoiseSIMD::SimplexFractal);
    noise->SetFrequency(0.005);
    noise->SetFractalType(FastNoiseSIMD::FBM);
    noise->SetFractalOctaves(5);
    noise->SetFractalLacunarity(2.0);
    noise->SetPerturbType(FastNoiseSIMD::Gradient);
    noise->SetPerturbAmp(0.4);
    noise->SetPerturbFrequency(0.4);
  }

  void generateTerrain(std::vector<uint8_t>& voxels) {
    float* noiseMap = noise->GetSampledNoiseSet(0, 0, 0, CS_P, CS_P, CS_P, 2);

    for (int i = CS_P3; i--;) {
      float noise = (noiseMap[i] + 1.0f) / 2.0f;

      int y = i / CS_P2;

      if (noise > 0.5f) {
          voxels.at(i) = 1;
      }
    }

    FastNoiseSIMD::FreeNoiseSet(noiseMap);
  }

  FastNoiseSIMD* noise;
};

#endif