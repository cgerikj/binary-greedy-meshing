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

  // Vertex data is packed into one unsigned integer:
  // - x, y, z: 6 bit each (0-63)
  // - Type: 8 bit (0-255)
  // - Normal: 3 bit (0-5)
  //
  // Meshes can be offset to world space using a per-draw uniform or by packing xyz
  // in gl_BaseInstance if rendering with glMultiDrawArraysIndirect.
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
  else return c + (b * CS_P) + (a * CS_P2);
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

  //Hidden face culling
  for (int a = 1; a < CS_P - 1; a++) {
    for (int b = 1; b < CS_P - 1; b++) {
      uint64_t cb = opaqueMask[(a * CS_P) + b] & P_MASK;

      faceMasks[(b - 1) + (a - 1) * CS + 0 * CS_2] = (cb & ~opaqueMask[(a + 1) * CS_P + b]) >> 1;
      faceMasks[(b - 1) + (a - 1) * CS + 1 * CS_2] = (cb & ~opaqueMask[(a - 1) * CS_P + b]) >> 1;

      faceMasks[(a - 1) + (b - 1) * CS + 2 * CS_2] = (cb & ~opaqueMask[a * CS_P + (b + 1)]) >> 1;
      faceMasks[(a - 1) + (b - 1) * CS + 3 * CS_2] = (cb & ~opaqueMask[a * CS_P + (b - 1)]) >> 1;

      faceMasks[(a - 1) + (b - 1) * CS + 4 * CS_2] = cb & ~(opaqueMask[a * CS_P + b] >> 1);
      faceMasks[(a - 1) + (b - 1) * CS + 5 * CS_2] = cb & ~(opaqueMask[a * CS_P + b] << 1);
    }
  }

  //Greedy meshing faces 0-3
  uint8_t forwardMerged[CS_2] { 0 };
  for (int face = 0; face < 4; face++) {
    int axis = face / 2;

    int faceVertexBegin = vertexI;

    for (int layer = 0; layer < CS; layer++) {
      int bitsLocation = layer * CS + face * CS_2;
      for (int forward = 0; forward < CS; forward++) {
        uint64_t bitsHere = faceMasks[forward + bitsLocation];
        uint64_t bitsNext = forward + 1 < CS ? faceMasks[(forward + 1) + bitsLocation] : 0;

        uint8_t rightMerged = 1;
        while (bitsHere) {
          unsigned long bitPos;
          #ifdef _MSC_VER
            _BitScanForward64(&bitPos, bitsHere);
          #else
            bitPos = __builtin_ctzll(bitsHere);
          #endif

          uint8_t type = voxels[getAxisIndex(axis, forward + 1, bitPos + 1, layer + 1)];

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

          uint8_t meshFront = forward - forwardMerged[bitPos];
          uint8_t meshLeft = bitPos;
          uint8_t meshUp = layer + (~face & 1);

          uint8_t meshWidth = rightMerged;
          uint8_t meshLength = forwardMerged[bitPos] + 1;

          forwardMerged[bitPos] = 0;
          rightMerged = 1;

          uint64_t quad;
          if (face == 0) {
            quad = getQuad(meshFront, meshUp, meshLeft, meshLength, meshWidth, type);
          } else if (face == 1) {
            quad = getQuad(meshFront, meshUp, meshLeft, meshLength, meshWidth, type);
          } else if (face == 2) {
            quad = getQuad(meshUp, meshFront, meshLeft, meshLength, meshWidth, type);
          } else if (face == 3) {
            quad = getQuad(meshUp, meshFront, meshLeft, meshLength, meshWidth, type);
          }

          insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
        }
      }
    }

    int faceVertexLength = vertexI - faceVertexBegin;
    meshData.faceVertexBegin[face] = faceVertexBegin;
    meshData.faceVertexLength[face] =faceVertexLength;
  }

  //Greedy meshing faces 4-5
  uint8_t rightMerged[CS] { 0 };
  for (int face = 4; face < 6; face++) {
    int axis = face / 2;

    int faceVertexBegin = vertexI;

    for (int forward = 0; forward < CS; forward++) {
      int bitsLocation = forward * CS + face * CS_2;
      int bitsForwardLocation = (forward + 1) * CS + face * CS_2;
      for (int right = 0; right < CS; right++) {
        uint64_t bitsHere = faceMasks[right + bitsLocation];
        uint64_t bitsForward = forward == CS - 1 ? 0 : faceMasks[right + bitsForwardLocation];
        uint64_t bitsRight = right == CS - 1 ? 0 : faceMasks[right + 1 + bitsLocation];

        while (bitsHere) {
          unsigned long bitPos;
          #ifdef _MSC_VER
            _BitScanForward64(&bitPos, bitsHere);
          #else
            bitPos = __builtin_ctzll(bitsHere);
          #endif

          bitsHere &= ~(1ull << bitPos);

          uint8_t type = voxels[getAxisIndex(axis, right + 1, forward + 1, bitPos)];

          if (rightMerged[bitPos - 1] == 0 && (bitsForward >> bitPos & 1) && type == voxels[getAxisIndex(axis, right + 1, forward + 2, bitPos)]) {
            forwardMerged[right * CS + (bitPos - 1)]++;
            continue;
          }

          if ((bitsRight >> bitPos & 1) && forwardMerged[right * CS + (bitPos - 1)] == forwardMerged[(right + 1) * CS + (bitPos - 1)]
            && type == voxels[getAxisIndex(axis, right + 2, forward + 1, bitPos)]) {
            forwardMerged[right * CS + (bitPos - 1)] = 0;
            rightMerged[bitPos - 1]++;
            continue;
          }

          uint8_t meshLeft = right - rightMerged[bitPos - 1];
          uint8_t meshFront = forward - forwardMerged[right * CS + bitPos - 1];
          uint8_t meshUp = bitPos - 1 + (~face & 1);

          uint8_t meshWidth = 1 + rightMerged[bitPos - 1];
          uint8_t meshLength = 1 + forwardMerged[right * CS + bitPos - 1];

          forwardMerged[right * CS + (bitPos - 1)] = 0;
          rightMerged[bitPos - 1] = 0;

          uint64_t quad;
          if (face == 4) {
            quad = getQuad(meshFront, meshLeft, meshUp, meshLength, meshWidth, type);
          } else if (face == 5) {
            quad = getQuad(meshFront, meshLeft, meshUp, meshLength, meshWidth, type);
          }

          insertQuad(*meshData.vertices, quad, vertexI, meshData.maxVertices);
        }
      }
    }
  
    int faceVertexLength = vertexI - faceVertexBegin;
    meshData.faceVertexBegin[face] = faceVertexBegin;
    meshData.faceVertexLength[face] =faceVertexLength;
  }

  meshData.vertexCount = vertexI + 1;
}

#endif // BM_IMPLEMENTATION
