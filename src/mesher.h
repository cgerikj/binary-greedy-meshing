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
  BM_VECTOR<uint64_t>* col_face_masks = nullptr; // CS_P2 * 6
  BM_VECTOR<uint64_t>* a_axis_cols = nullptr; // CS_P2
  BM_VECTOR<uint64_t>* b_axis_cols = nullptr; // CS_P
  BM_VECTOR<uint64_t>* merged_right = nullptr; // CS_P
  BM_VECTOR<uint64_t>* merged_forward = nullptr; // CS_P2

  // Vertex data is packed into one unsigned integer:
  // - x, y, z: 6 bit each (0-63)
  // - Type: 8 bit (0-255)
  // - Normal: 3 bit (0-5)
  // - AO: 2 bit
  //
  // Meshes can be offset to world space using a per-draw uniform or by packing xyz
  // in gl_BaseInstance if rendering with glMultiDrawArraysIndirect.
  BM_VECTOR<uint32_t>* vertices = nullptr;
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
void mesh(const BM_VECTOR<uint8_t>& voxels, MeshData& meshData, bool bake_ao = true);

#endif // MESHER_H

#ifdef BM_IMPLEMENTATION

#ifndef BM_MEMSET
#define BM_MEMSET memset
#include <string.h> // memset
#endif

static inline const int get_axis_i(const int axis, const int a, const int b, const int c) {
  if (axis == 0) return b + (a * CS_P) + (c * CS_P2);
  else if (axis == 1) return a + (c * CS_P) + (b* CS_P2);
  else return c + (b * CS_P) + (a * CS_P2);
}

// Add checks to this function to skip culling against grass for example
static inline const bool solid_check(int voxel) {
  return voxel > 0;
}

static const BM_IVEC2 ao_dirs[8] = {
  BM_IVEC2(-1, 0),
  BM_IVEC2(0, -1),
  BM_IVEC2(0, 1),
  BM_IVEC2(1, 0),
  BM_IVEC2(-1, -1),
  BM_IVEC2(-1, 1),
  BM_IVEC2(1, -1),
  BM_IVEC2(1, 1),
};

static inline const int vertexAO(uint8_t side1, uint8_t side2, uint8_t corner) {
  return (side1 && side2) ? 0 : (3 - (side1 + side2 + corner));
}

static inline const bool compare_ao(const BM_VECTOR<uint8_t>& voxels, int axis, int forward, int right, int c, int forward_offset, int right_offset) {
  for (auto& ao_dir : ao_dirs) {
    if (solid_check(voxels[get_axis_i(axis, right + ao_dir[0], forward + ao_dir[1], c)]) !=
      solid_check(voxels[get_axis_i(axis, right + right_offset + ao_dir[0], forward + forward_offset + ao_dir[1], c)])
    ) {
      return false;
    }
  }
  return true;
}

static inline const void insert_quad(BM_VECTOR<uint32_t>& vertices, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, bool flipped, int& vertexI, int& maxVertices) {
  if (vertexI >= maxVertices - 6) {
    vertices.resize(maxVertices * 2, 0);
    maxVertices *= 2;
  }

  if (flipped) {
    vertices[vertexI] = v1;
    vertices[vertexI + 1] = v2;
    vertices[vertexI + 2] = v3;
    vertices[vertexI + 3] = v3;
    vertices[vertexI + 4] = v4;
    vertices[vertexI + 5] = v1;
  }
  else {
    vertices[vertexI] = v1;
    vertices[vertexI + 1] = v2;
    vertices[vertexI + 2] = v4;
    vertices[vertexI + 3] = v4;
    vertices[vertexI + 4] = v2;
    vertices[vertexI + 5] = v3;
  }

  vertexI += 6;
}

static inline const uint32_t get_vertex(uint32_t x, uint32_t y, uint32_t z, uint32_t type, uint32_t norm, uint32_t ao) {
  return (ao << 29) | (norm << 26) | (type << 18) | ((z - 1) << 12) | ((y - 1) << 6) | (x - 1);
}

static const uint64_t CULL_MASK = (1ULL << (CS_P - 1));
static const uint64_t BORDER_MASK = (1ULL | (1ULL <<  (CS_P - 1)));

void mesh(const BM_VECTOR<uint8_t>& voxels, MeshData& meshData, bool bake_ao) {
  meshData.vertexCount = 0;
  int vertexI = 0;

  auto col_face_masks = *meshData.col_face_masks;
  auto a_axis_cols = *meshData.a_axis_cols;
  auto b_axis_cols = *meshData.b_axis_cols;
  auto merged_right = *meshData.merged_right;
  auto merged_forward = *meshData.merged_forward;

  // Begin culling faces
  auto p = voxels.begin();
  BM_MEMSET(a_axis_cols.data(), 0, CS_P2 * sizeof(uint64_t));
  for (int a = 0; a < CS_P; a++) {
    BM_MEMSET(b_axis_cols.data(), 0, CS_P * sizeof(uint64_t));

    for (int b = 0; b < CS_P; b++) {
      uint64_t cb = 0;

      for (int c = 0; c < CS_P; c++) {
        if (solid_check(*p)) {
          a_axis_cols[b + (c * CS_P)] |= 1ULL << a;
          b_axis_cols[c] |= 1ULL << b;
          cb |= 1ULL << c;
        }
        p++;
      }

      // Cull third axis faces
      col_face_masks[a + (b * CS_P) + (4 * CS_P2)] = cb & ~((cb >> 1) | CULL_MASK);
      col_face_masks[a + (b * CS_P) + (5 * CS_P2)] = cb & ~((cb << 1) | 1ULL);
    }

    // Cull second axis faces
    int faceIndex = (a * CS_P) + (2 * CS_P2);
    for (int b = 1; b < CS_P - 1; b++) {
      uint64_t col = b_axis_cols[b];
      col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
      col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
    }
  }

  // Cull first axis faces
  for (int a = 1; a < CS_P - 1; a++) {
    int faceIndex = a * CS_P;
    for (int b = 1; b < CS_P - 1; b++) {
      uint64_t col = a_axis_cols[faceIndex + b];

      col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
      col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
    }
  }

  // Greedy meshing
  for (uint8_t face = 0; face < 6; face++) {
    int axis = face / 2;
    int air_dir = face % 2 == 0 ? 1 : -1;

    BM_MEMSET(merged_forward.data(), 0, CS_P2 * 8);

    for (int forward = 1; forward < CS_P - 1; forward++) {
      uint64_t bits_walking_right = 0;
      int forwardIndex = (forward * CS_P) + (face * CS_P2);

      BM_MEMSET(merged_right.data(), 0, CS_P * 8);

      for (int right = 1; right < CS_P - 1; right++) {
        int rightxCS_P = right * CS_P;

        uint64_t bits_here = col_face_masks[forwardIndex + right] &~ BORDER_MASK;
        uint64_t bits_right = right >= CS ? 0 : col_face_masks[forwardIndex + right + 1];
        uint64_t bits_forward = forward >= CS ? 0 : col_face_masks[forwardIndex + right + CS_P];

        uint64_t bits_merging_forward = bits_here & bits_forward & ~bits_walking_right;
        uint64_t bits_merging_right = bits_here & bits_right;

        unsigned long bit_pos;

        uint64_t copy_front = bits_merging_forward;
        while (copy_front) {
          #ifdef _MSC_VER
            _BitScanForward64(&bit_pos, copy_front);
          #else
            bit_pos = __builtin_ctzll(copy_front);
          #endif

          copy_front &= ~(1ULL << bit_pos);

          if(
            voxels[get_axis_i(axis, right, forward, bit_pos)] == voxels[get_axis_i(axis, right, forward + 1, bit_pos)] &&
            (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 1, 0))
          ) {
            merged_forward[(right * CS_P) + bit_pos]++;
          }
          else {
            bits_merging_forward &= ~(1ULL << bit_pos);
          }
        }

        uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
        while (bits_stopped_forward) {
          #ifdef _MSC_VER
            _BitScanForward64(&bit_pos, bits_stopped_forward);
          #else
            bit_pos = __builtin_ctzll(bits_stopped_forward);
          #endif

          bits_stopped_forward &= ~(1ULL << bit_pos);

          uint8_t type = voxels[get_axis_i(axis, right, forward, bit_pos)];

          if (
            (bits_merging_right & (1ULL << bit_pos)) != 0 &&
            (merged_forward[(right * CS_P) + bit_pos] == merged_forward[(right + 1) * CS_P + bit_pos]) &&
            (type == voxels[get_axis_i(axis, right + 1, forward, bit_pos)]) &&
            (!bake_ao || compare_ao(voxels, axis, forward, right, bit_pos + air_dir, 0, 1))
            ) {
            bits_walking_right |= 1ULL << bit_pos;
            merged_right[bit_pos]++;
            merged_forward[rightxCS_P + bit_pos] = 0;
            continue;
          }

          bits_walking_right &= ~(1ULL << bit_pos);

          uint8_t mesh_left = right - merged_right[bit_pos];
          uint8_t mesh_right = right + 1;
          uint8_t mesh_front = forward - merged_forward[rightxCS_P + bit_pos];
          uint8_t mesh_back = forward + 1;
          uint8_t mesh_up = bit_pos + (face % 2 == 0 ? 1 : 0);

          uint8_t ao_LB = 3, ao_RB = 3, ao_RF = 3, ao_LF = 3;
          if (bake_ao) {
            int c = bit_pos + air_dir;
            uint8_t ao_F = solid_check(voxels[get_axis_i(axis, right, forward - 1, c)]);
            uint8_t ao_B = solid_check(voxels[get_axis_i(axis, right, forward + 1, c)]);
            uint8_t ao_L = solid_check(voxels[get_axis_i(axis, right - 1, forward, c)]);
            uint8_t ao_R = solid_check(voxels[get_axis_i(axis, right + 1, forward, c)]);

            uint8_t ao_LFC = !ao_L && !ao_F && solid_check(voxels[get_axis_i(axis, right - 1, forward - 1, c)]);
            uint8_t ao_LBC = !ao_L && !ao_B && solid_check(voxels[get_axis_i(axis, right - 1, forward + 1, c)]);
            uint8_t ao_RFC = !ao_R && !ao_F && solid_check(voxels[get_axis_i(axis, right + 1, forward - 1, c)]);
            uint8_t ao_RBC = !ao_R && !ao_B && solid_check(voxels[get_axis_i(axis, right + 1, forward + 1, c)]);

            ao_LB = vertexAO(ao_L, ao_B, ao_LBC);
            ao_RB = vertexAO(ao_R, ao_B, ao_RBC);
            ao_RF = vertexAO(ao_R, ao_F, ao_RFC);
            ao_LF = vertexAO(ao_L, ao_F, ao_LFC);
          }

          merged_forward[rightxCS_P + bit_pos] = 0;
          merged_right[bit_pos] = 0;

          uint32_t v1, v2, v3, v4;
          if (face == 0) {
            v1 = get_vertex(mesh_left, mesh_up, mesh_back, type, face, ao_LB);
            v2 = get_vertex(mesh_right, mesh_up, mesh_back, type, face, ao_RB);
            v3 = get_vertex(mesh_right, mesh_up, mesh_front, type, face, ao_RF);
            v4 = get_vertex(mesh_left, mesh_up, mesh_front, type, face, ao_LF);
          }
          else if (face == 1) {
            v1 = get_vertex(mesh_left, mesh_up, mesh_back, type, face, ao_LB);
            v2 = get_vertex(mesh_left, mesh_up, mesh_front, type, face, ao_LF);
            v3 = get_vertex(mesh_right, mesh_up, mesh_front, type, face, ao_RF);
            v4 = get_vertex(mesh_right, mesh_up, mesh_back, type, face, ao_RB);
          }
          else if (face == 2) {
            v1 = get_vertex(mesh_up, mesh_back, mesh_left, type, face, ao_LB);
            v2 = get_vertex(mesh_up, mesh_back, mesh_right, type, face, ao_RB);
            v3 = get_vertex(mesh_up, mesh_front, mesh_right, type, face, ao_RF);
            v4 = get_vertex(mesh_up, mesh_front, mesh_left, type, face, ao_LF);
          }
          else if (face == 3) {
            v1 = get_vertex(mesh_up, mesh_back, mesh_left, type, face, ao_LB);
            v2 = get_vertex(mesh_up, mesh_front, mesh_left, type, face, ao_LF);
            v3 = get_vertex(mesh_up, mesh_front, mesh_right, type, face, ao_RF);
            v4 = get_vertex(mesh_up, mesh_back, mesh_right, type, face, ao_RB);
          }
          else if (face == 4) {
            v1 = get_vertex(mesh_back, mesh_left, mesh_up, type, face, ao_LB);
            v2 = get_vertex(mesh_back, mesh_right, mesh_up, type, face, ao_RB);
            v3 = get_vertex(mesh_front, mesh_right, mesh_up, type, face, ao_RF);
            v4 = get_vertex(mesh_front, mesh_left, mesh_up, type, face, ao_LF);
          }
          else if (face == 5) {
            v1 = get_vertex(mesh_back, mesh_left, mesh_up, type, face, ao_LB);
            v2 = get_vertex(mesh_front, mesh_left, mesh_up, type, face, ao_LF);
            v3 = get_vertex(mesh_front, mesh_right, mesh_up, type, face, ao_RF);
            v4 = get_vertex(mesh_back, mesh_right, mesh_up, type, face, ao_RB);
          }

          insert_quad(*meshData.vertices, v1, v2, v3, v4, ao_LB + ao_RF > ao_RB + ao_LF, vertexI, meshData.maxVertices);
        }
      }
    }
  }

  meshData.vertexCount = vertexI + 1;
}
#endif // BM_IMPLEMENTATION
