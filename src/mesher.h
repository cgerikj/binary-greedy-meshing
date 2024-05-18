/*
MIT License

Copyright (c) 2020 Erik Johansson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

//   Define BM_IMPLEMENTATION in exactly one source file to include the implementation
//   and include the mesher.h without that define as often as needed
//
//   it could look like this
//
//   #define BM_IMPLEMENTATION
//   #include "mesher.h"
//
//   There are other defines to control the behaviour of the library.
//   * Define BM_IVEC2 with your own vector implementation - otherwise it will use glm::ivec2
//   * Define BM_VECTOR with your own vector implementation - otherwise it will use std::vector

#ifndef MESHER_H
#define MESHER_H

#ifndef BM_IVEC2
#include <glm/glm.hpp>
#define BM_IVEC2 glm::ivec2
#endif

#ifndef BM_VECTOR
#include <vector>
#define BM_VECTOR std::vector
#endif

#include <stdint.h>

// CS = chunk size (max 62)
static constexpr int CS = 62;

// Padded chunk size
static constexpr int CS_P = CS + 2;
static constexpr int CS_2 = CS * CS;
static constexpr int CS_P2 = CS_P * CS_P;
static constexpr int CS_P3 = CS_P * CS_P * CS_P;

struct MeshData {
  uint64_t* faceMasks = nullptr; // CS_2 * 6
  uint64_t* opaqueMask = nullptr; //CS_P2
  uint8_t* forwardMerged = nullptr; // CS_2
  uint8_t* rightMerged = nullptr; // CS
  BM_VECTOR<uint64_t>* vertices = nullptr;
  int vertexCount = 0;
  int maxVertices = 0;
  int faceVertexBegin[6] = { 0 };
  int faceVertexLength[6] = { 0 };
};

// voxels - 64^3 (includes neighboring voxels)
// vertices - pre-allocated array of vertices that will be poplulated. Can be re-used between runs and does not need to be clared.
// vertexLength - output  number of vertices to read from vertices
//
// @param[in] voxels The input data includes duplicate edge data from neighboring chunks which is used
// for visibility culling. For optimal performance, your world data should already be structured
// this way so that you can feed the data straight into this algorithm.
// Input data is ordered in YXZ and is 64^3 which results in a 62^3 mesh.
//
// @param[out] meshData The allocated vertices in MeshData with a length of meshData.vertexCount.
void mesh(const uint8_t* voxels, MeshData& meshData);

#endif // MESHER_H

#ifdef BM_IMPLEMENTATION

#ifndef BM_MEMSET
#define BM_MEMSET memset
#include <string.h> // memset
#endif

static inline const int getAxisIndex(const int axis, const int a, const int b, const int c) {
  if (axis == 0) return b + (a * CS_P) + (c * CS_P2);
  else if (axis == 1) return b + (c * CS_P) + (a * CS_P2);
  else return c + (a * CS_P) + (b * CS_P2);
}

static inline const void insertQuad(BM_VECTOR<uint64_t>& vertices, uint64_t quad, int& vertexI, int& maxVertices) {
  if (vertexI >= maxVertices - 6) {
    vertices.resize(maxVertices * 2, 0);
    maxVertices *= 2;
  }

  vertices[vertexI] = quad;

  vertexI++;
}

static inline const uint64_t getQuad(uint64_t x, uint64_t y, uint64_t z, uint64_t w, uint64_t h, uint64_t type) {
  return (type << 32) | (h << 24) | (w << 18) | (z << 12) | (y << 6) | x;
}

constexpr uint64_t P_MASK = ~(1ull << 63 | 1);

void mesh(const uint8_t* voxels, MeshData& meshData) {
  meshData.vertexCount = 0;
  int vertexI = 0;

  uint64_t* opaqueMask = meshData.opaqueMask;
  uint64_t* faceMasks = meshData.faceMasks;
  uint8_t* forwardMerged = meshData.forwardMerged;
  uint8_t* rightMerged = meshData.rightMerged;

  //Hidden face culling
  for (int a = 1; a < CS_P - 1; a++) {
    for (int b = 1; b < CS_P - 1; b++) {
        const uint64_t columnB = opaqueMask[(a * CS_P) + b] & P_MASK;
        const int baIndex = (b - 1) + (a - 1) * CS;
        const int abIndex = (a - 1) + (b - 1) * CS;
        const int aChunkSizePadding = a * CS_P;

        faceMasks[baIndex + 0 * CS_2] = (columnB & ~opaqueMask[(a + 1) * CS_P + b]) >> 1;
        faceMasks[baIndex + 1 * CS_2] = (columnB & ~opaqueMask[(a - 1) * CS_P + b]) >> 1;

        faceMasks[abIndex + 2 * CS_2] = (columnB & ~opaqueMask[aChunkSizePadding + (b + 1)]) >> 1;
        faceMasks[abIndex + 3 * CS_2] = (columnB & ~opaqueMask[aChunkSizePadding + (b - 1)]) >> 1;

        faceMasks[baIndex + 4 * CS_2] = columnB & ~(opaqueMask[aChunkSizePadding + b] >> 1);
        faceMasks[baIndex + 5 * CS_2] = columnB & ~(opaqueMask[aChunkSizePadding + b] << 1);
    }
  }

  //Greedy meshing faces 0-3
  for (int face = 0; face < 4; face++) {
    const int axis = face / 2;

    const int faceVertexBegin = vertexI;

    for (int layer = 0; layer < CS; layer++) {
      const int bitsLocation = layer * CS + face * CS_2;
      uint64_t* faceMasksPtr = &faceMasks[bitsLocation];

      for (int forward = 0; forward < CS; forward++) {
        uint64_t bitsHere = faceMasksPtr[forward];
        const uint64_t bitsNext = forward + 1 < CS ? faceMasksPtr[forward + 1] : 0;

        uint8_t rightMerged = 1;
        while (bitsHere) {
          unsigned long bitPos;
          #ifdef _MSC_VER
            _BitScanForward64(&bitPos, bitsHere);
          #else
            bitPos = __builtin_ctzll(bitsHere);
          #endif

          const uint8_t type = voxels[getAxisIndex(axis, forward + 1, bitPos + 1, layer + 1)];

          if ((bitsNext >> bitPos & 1) && type == voxels[getAxisIndex(axis, forward + 2, bitPos + 1, layer + 1)]) {
            forwardMerged[bitPos]++;
            bitsHere &= ~(1ull << bitPos);
            continue;
          }

          for (int right = bitPos + 1; right < CS; right++) {
            if (!(bitsHere >> right & 1) || forwardMerged[bitPos] != forwardMerged[right] || type != voxels[getAxisIndex(axis, forward + 1, right + 1, layer + 1)]) break;
            forwardMerged[right] = 0;
            rightMerged++;
          }
          bitsHere &= ~((1ull << (bitPos + rightMerged)) - 1);

          const uint8_t meshFront = forward - forwardMerged[bitPos];
          const uint8_t meshLeft = bitPos;
          const uint8_t meshUp = layer + (~face & 1);

          const uint8_t meshWidth = rightMerged;
          const uint8_t meshLength = forwardMerged[bitPos] + 1;

          forwardMerged[bitPos] = 0;
          rightMerged = 1;

          uint64_t quad;
          switch (face) {
          case 0:
          case 1:
            quad = getQuad(meshFront + (face == 1 ? meshLength : 0), meshUp, meshLeft, meshLength, meshWidth, type);
            break;
          case 2:
          case 3:
            quad = getQuad(meshUp, meshFront + (face == 2 ? meshLength : 0), meshLeft, meshLength, meshWidth, type);
            break;
          }

          insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
        }
      }
    }

    const int faceVertexLength = vertexI - faceVertexBegin;
    meshData.faceVertexBegin[face] = faceVertexBegin;
    meshData.faceVertexLength[face] =faceVertexLength;
  }

  //Greedy meshing faces 4-5
  for (int face = 4; face < 6; face++) {
    const int axis = face / 2;

    const int faceVertexBegin = vertexI;

    for (int forward = 0; forward < CS; forward++) {
      const int bitsLocation = forward * CS + face * CS_2;
      const int bitsForwardLocation = (forward + 1) * CS + face * CS_2;

      uint64_t* faceMasksPtr = &faceMasks[bitsLocation];
      uint64_t* faceMasksPtrForward = &faceMasks[bitsForwardLocation];

      for (int right = 0; right < CS; right++) {
        uint64_t bitsHere = faceMasksPtr[right];
        const uint64_t bitsForward = forward == CS - 1 ? 0 : faceMasksPtrForward[right];
        const uint64_t bitsRight = right == CS - 1 ? 0 : faceMasksPtr[right + 1];
        const int rightTimesCS = right * CS;

        while (bitsHere) {
          unsigned long bitPos;
          #ifdef _MSC_VER
            _BitScanForward64(&bitPos, bitsHere);
          #else
            bitPos = __builtin_ctzll(bitsHere);
          #endif

          bitsHere &= ~(1ull << bitPos);

          const uint8_t type = voxels[getAxisIndex(axis, right + 1, forward + 1, bitPos)];

          if (rightMerged[bitPos - 1] == 0 && (bitsForward >> bitPos & 1) && type == voxels[getAxisIndex(axis, right + 1, forward + 2, bitPos)]) {
            forwardMerged[rightTimesCS + (bitPos - 1)]++;
            continue;
          }

          if ((bitsRight >> bitPos & 1) && forwardMerged[rightTimesCS + (bitPos - 1)] == forwardMerged[(rightTimesCS - right) + (bitPos - 1)]
            && type == voxels[getAxisIndex(axis, right + 2, forward + 1, bitPos)]) {
            forwardMerged[rightTimesCS + (bitPos - 1)] = 0;
            rightMerged[bitPos - 1]++;
            continue;
          }

          const uint8_t meshLeft = right - rightMerged[bitPos - 1];
          const uint8_t meshFront = forward - forwardMerged[rightTimesCS + bitPos - 1];
          const uint8_t meshUp = bitPos - 1 + (~face & 1);

          const uint8_t meshWidth = 1 + rightMerged[bitPos - 1];
          const uint8_t meshLength = 1 + forwardMerged[rightTimesCS + bitPos - 1];

          forwardMerged[rightTimesCS + (bitPos - 1)] = 0;
          rightMerged[bitPos - 1] = 0;

          const uint64_t quad = getQuad(meshLeft + (face == 4 ? meshWidth : 0), meshFront, meshUp, meshWidth, meshLength, type);

          insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
        }
      }
    }
  
    const int faceVertexLength = vertexI - faceVertexBegin;
    meshData.faceVertexBegin[face] = faceVertexBegin;
    meshData.faceVertexLength[face] =faceVertexLength;
  }

  meshData.vertexCount = vertexI + 1;
}

#endif // BM_IMPLEMENTATION
