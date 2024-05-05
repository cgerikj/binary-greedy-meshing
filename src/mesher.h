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

#ifndef MESHER_H
#define MESHER_H

#include <glm/glm.hpp>
#include <vector>
#include <stdint.h>
#include <string.h> // memset

#include "constants.h"

static inline const int get_axis_i(const int axis, const int a, const int b, const int c) {
  if (axis == 0) return b + (a * CS_P) + (c * CS_P2);
  else if (axis == 1) return b + (c * CS_P) + (a * CS_P2);
  else return c + (b * CS_P) + (a * CS_P2);
}

static inline const void insert_quad(std::vector<uint32_t>& vertices, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, int& vertexI, int& maxVertices) {
  if (vertexI >= maxVertices - 6) {
    vertices.resize(maxVertices * 2, 0);
    maxVertices *= 2;
  }

  vertices[vertexI] = v1;
  vertices[vertexI + 1] = v2;
  vertices[vertexI + 2] = v4;
  vertices[vertexI + 3] = v4;
  vertices[vertexI + 4] = v2;
  vertices[vertexI + 5] = v3;

  vertexI += 6;
}

static inline const uint32_t get_vertex(uint32_t x, uint32_t y, uint32_t z, uint32_t type, uint32_t norm) {
  return (3u << 29) | (norm << 26) | (type << 18) | (z << 12) | (y << 6) | x;
}

struct MeshData {
  uint64_t* col_face_masks = nullptr; // CS_2 * 6
  uint64_t* axis_cols = nullptr; //CS_P2

  // Vertex data is packed into one unsigned integer:
  // - x, y, z: 6 bit each (0-63)
  // - Type: 8 bit (0-255)
  // - Normal: 3 bit (0-5)
  // - AO: 2 bit
  //
  // Meshes can be offset to world space using a per-draw uniform or by packing xyz
  // in gl_BaseInstance if rendering with glMultiDrawArraysIndirect.
  std::vector<uint32_t>* vertices = nullptr;
  int vertexCount = 0;
  int maxVertices = 0;
};

// voxels - 64^3 (includes neighboring voxels)
// vertices - pre-allocated array of vertices that will be poplulated. Can be re-used between runs and does not need to be clared.
// vertexLength - output  number of vertices to read from vertices
//
// @param[in] voxels The input data includes duplicate edge data from neighboring chunks which is used
// for visibility culling and AO. For optimal performance, your world data should already be structured
// this way so that you can feed the data straight into this algorithm.
// Input data is ordered in YXZ and is 64^3 which results in a 62^3 mesh.
//
// @param[out] meshData The allocated vertices in MeshData with a length of meshData.vertexCount.
//
// @param[in] bake_ao true if you want baked ambient occlusion.
constexpr uint64_t P_MASK = ~(1ull << 63 | 1);
void mesh(const uint8_t* voxels, MeshData& meshData, bool bake_ao) {
  meshData.vertexCount = 0;
  int vertexI = 0;

  uint64_t* axis_cols = meshData.axis_cols;
  uint64_t* col_face_masks = meshData.col_face_masks;

  for (int a = 1; a < CS_P - 1; a++) {
    for (int b = 1; b < CS_P - 1; b++) {
      uint64_t cb = axis_cols[(a * CS_P) + b] & P_MASK;

      col_face_masks[(b - 1) + (a - 1) * CS + 0 * CS_2] = (cb & ~axis_cols[(a + 1) * CS_P + b]) >> 1;
      col_face_masks[(b - 1) + (a - 1) * CS + 1 * CS_2] = (cb & ~axis_cols[(a - 1) * CS_P + b]) >> 1;

      col_face_masks[(a - 1) + (b - 1) * CS + 2 * CS_2] = (cb & ~axis_cols[a * CS_P + (b + 1)]) >> 1;
      col_face_masks[(a - 1) + (b - 1) * CS + 3 * CS_2] = (cb & ~axis_cols[a * CS_P + (b - 1)]) >> 1;

      col_face_masks[(a - 1) + (b - 1) * CS + 4 * CS_2] = cb & ~(axis_cols[a * CS_P + b] >> 1);
      col_face_masks[(a - 1) + (b - 1) * CS + 5 * CS_2] = cb & ~(axis_cols[a * CS_P + b] << 1);
    }
  }

  //Greedy meshing faces 0-3
  uint8_t forwardMerged[CS_2] { 0 };
  for (int face = 0; face < 4; face++) {
    int axis = face / 2;

    for (int layer = 0; layer < CS; layer++) {
      int bitsLocation = layer * CS + face * CS_2;
      for (int forward = 0; forward < CS; forward++) {
        uint64_t bitsHere = col_face_masks[forward + bitsLocation];
        uint64_t bitsNext = forward + 1 < CS ? col_face_masks[(forward + 1) + bitsLocation] : 0;

        uint8_t rightMerged = 1;
        while (bitsHere) {
          unsigned long bitPos;
#ifdef _MSC_VER
          _BitScanForward64(&bitPos, bitsHere);
#else
          bitPos = __builtin_ctzll(bitsHere);
#endif

          uint8_t type = voxels[get_axis_i(axis, forward + 1, bitPos + 1, layer + 1)];

          if ((bitsNext >> bitPos & 1) && type == voxels[get_axis_i(axis, forward + 2, bitPos + 1, layer + 1)]) {
            forwardMerged[bitPos]++;
            bitsHere &= ~(1ull << bitPos);
            continue;
          }

          for (int right = bitPos + 1; right < CS; right++) {
            if (!(bitsHere >> right & 1) || forwardMerged[bitPos] != forwardMerged[right] || type != voxels[get_axis_i(axis, forward + 1, right + 1, layer + 1)]) break;
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

          uint32_t v1, v2, v3, v4;
          if (face == 0) {
            v1 = get_vertex(meshFront, meshUp, meshLeft, type, face);
            v2 = get_vertex(meshFront, meshUp, meshLeft + meshWidth, type, face);
            v3 = get_vertex(meshFront + meshLength, meshUp, meshLeft + meshWidth, type, face);
            v4 = get_vertex(meshFront + meshLength, meshUp, meshLeft, type, face);
          } else if (face == 1) {
            v1 = get_vertex(meshFront + meshLength, meshUp, meshLeft + meshWidth, type, face);
            v2 = get_vertex(meshFront, meshUp, meshLeft + meshWidth, type, face);
            v3 = get_vertex(meshFront, meshUp, meshLeft, type, face);
            v4 = get_vertex(meshFront + meshLength, meshUp, meshLeft, type, face);
          } else if (face == 2) {
            v1 = get_vertex(meshUp, meshFront + meshLength, meshLeft, type, face);
            v2 = get_vertex(meshUp, meshFront + meshLength, meshLeft + meshWidth, type, face);
            v3 = get_vertex(meshUp, meshFront, meshLeft + meshWidth, type, face);
            v4 = get_vertex(meshUp, meshFront, meshLeft, type, face);
          } else if (face == 3) {
            v1 = get_vertex(meshUp, meshFront + meshLength, meshLeft, type, face);
            v2 = get_vertex(meshUp, meshFront, meshLeft, type, face);
            v3 = get_vertex(meshUp, meshFront, meshLeft + meshWidth, type, face);
            v4 = get_vertex(meshUp, meshFront + meshLength, meshLeft + meshWidth, type, face);
          }

          insert_quad(*meshData.vertices, v1, v2, v3, v4, vertexI, meshData.maxVertices);
        }
      }
    }
  }

  //Greedy meshing faces 4-5
  uint8_t rightMerged[CS] { 0 };
  for (int face = 4; face < 6; face++) {
    int axis = face / 2;

    for (int forward = 0; forward < CS; forward++) {
      int bitsLocation = forward * CS + face * CS_2;
      int bitsForwardLocation = (forward + 1) * CS + face * CS_2;
      for (int right = 0; right < CS; right++) {
        uint64_t bitsHere = col_face_masks[right + bitsLocation];
        uint64_t bitsForward = forward == CS - 1 ? 0 : col_face_masks[right + bitsForwardLocation];
        uint64_t bitsRight = right == CS - 1 ? 0 : col_face_masks[right + 1 + bitsLocation];

        while (bitsHere) {
          unsigned long bitPos;
#ifdef _MSC_VER
          _BitScanForward64(&bitPos, bitsHere);
#else
          bitPos = __builtin_ctzll(bitsHere);
#endif

          bitsHere &= ~(1ull << bitPos);

          uint8_t type = voxels[get_axis_i(axis, right + 1, forward + 1, bitPos)];

          if (rightMerged[bitPos - 1] == 0 && (bitsForward >> bitPos & 1) && type == voxels[get_axis_i(axis, right + 1, forward + 2, bitPos)]) {
            forwardMerged[right * CS + (bitPos - 1)]++;
            continue;
          }

          if ((bitsRight >> bitPos & 1) && forwardMerged[right * CS + (bitPos - 1)] == forwardMerged[(right + 1) * CS + (bitPos - 1)]
            && type == voxels[get_axis_i(axis, right + 2, forward + 1, bitPos)]) {
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

          uint32_t v1, v2, v3, v4;
          if (face == 4) {
            v1 = get_vertex(meshFront + meshLength, meshLeft, meshUp, type, face);
            v2 = get_vertex(meshFront + meshLength, meshLeft + meshWidth, meshUp, type, face);
            v3 = get_vertex(meshFront, meshLeft + meshWidth, meshUp, type, face);
            v4 = get_vertex(meshFront, meshLeft, meshUp, type, face);
          } else if (face == 5) {
            v1 = get_vertex(meshFront + meshLength, meshLeft, meshUp, type, face);
            v2 = get_vertex(meshFront, meshLeft, meshUp, type, face);
            v3 = get_vertex(meshFront, meshLeft + meshWidth, meshUp, type, face);
            v4 = get_vertex(meshFront + meshLength, meshLeft + meshWidth, meshUp, type, face);
          }

          insert_quad(*meshData.vertices, v1, v2, v3, v4, vertexI, meshData.maxVertices);
        }
      }
    }
  }

  meshData.vertexCount = vertexI + 1;
}

#endif