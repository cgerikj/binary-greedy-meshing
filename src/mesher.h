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

#ifdef _MSC_VER
inline const int CTZ(uint64_t& x) {
  unsigned long index;
  _BitScanForward64(&index, x);
  return (int)index;
}
#else
inline const int CTZ(uint64_t x) {
  return __builtin_ctzll(x);
}
#endif

#ifndef MESHER_H
#define MESHER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <stdint.h>
#include "timer.h"
#include "constants.h"

inline const uint16_t get_vertex(uint8_t x, uint8_t y, uint8_t z) {
  return  ((z - 1) << 10) | ((y - 1) << 5) | (x - 1);
}

static constexpr int CS_LAST_BIT = CS_P - 1;
static constexpr uint64_t EDGE_MASK = (1ULL << CS_LAST_BIT) + 1ULL;

void mesh(std::vector<uint8_t>& voxels, std::vector<uint16_t>* vertices) {

  uint64_t axis_cols[CS_P2 * 3] = { 0 };
  uint64_t col_face_masks[CS_P2 * 6];

  auto p = voxels.begin();
  for (int y = 0; y < CS_P; y++) {
    int y_offset = (y * CS_P) + CS_P2;
    for (int x = 0; x < CS_P; x++) {
      uint64_t zb = 0;
      for (int z = 0; z < CS_P; z++) {
        if (*p) {
          axis_cols[x + (z * CS_P)] |= 1ULL << y;
          axis_cols[z + y_offset] |= 1ULL << x;
          zb |= 1ULL << z;
        }
        p++;
      }
      axis_cols[y + (x * CS_P) + (CS_P2 * 2)] = zb;
    }
  }

  for (int axis = 0; axis <= 2; axis++) {
    for (int i = 0; i < CS_P2; i++) {
      uint64_t col = axis_cols[(CS_P2 * axis) + i];
      col_face_masks[(CS_P2 * (axis * 2)) + i] = col & ~((col >> 1) | (1ULL << CS_LAST_BIT));
      col_face_masks[(CS_P2 * (axis * 2 + 1)) + i] = col & ~((col << 1) | 1ULL);
    }
  }

  for (int face = 0; face < 6; face++) {
    int axis = face / 2;
    int facexCS_P2 = face * CS_P2;
    int faceEvenOffset = (face % 2 == 0) ? 1 : 0;

    int merged_forward[CS_P2] = { 0 };
    for (int forward = 1; forward < CS_LAST_BIT; forward++) {
      int forwardxCS_PxfacexCS_P2 = (forward * CS_P) + facexCS_P2;
      uint64_t bits_walking_right = 0;
      int merged_right[CS_P] = { 0 };
      bool forwardGteCS = forward >= CS;

      for (int right = 1; right < CS_LAST_BIT; right++) {
        int rightxCS_P = right * CS_P;

        uint64_t bits_here = col_face_masks[right + forwardxCS_PxfacexCS_P2] &~ EDGE_MASK;
        uint64_t bits_merging_forward = forwardGteCS ? 0 : (bits_here & ~bits_walking_right & col_face_masks[right + forwardxCS_PxfacexCS_P2 + CS_P]);

        uint64_t copy_front = bits_merging_forward;
        while (copy_front) {
          int bit_pos = CTZ(copy_front);
          copy_front &= ~(1ULL << bit_pos);

          merged_forward[rightxCS_P + bit_pos]++;
        }

        uint64_t bits_merging_right = right >= CS ? 0 : (bits_here & (col_face_masks[right + 1 + forwardxCS_PxfacexCS_P2]));

        uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
        while (bits_stopped_forward) {
          int bit_pos = CTZ(bits_stopped_forward);
          bits_stopped_forward &= ~(1ULL << bit_pos);

          if ((bits_merging_right & (1ULL << bit_pos)) && merged_forward[rightxCS_P + bit_pos] == merged_forward[(right + 1) * CS_P + bit_pos]) {
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
          uint8_t mesh_up = bit_pos + faceEvenOffset;

          merged_forward[rightxCS_P + bit_pos] = 0;
          merged_right[bit_pos] = 0;

          uint16_t v1, v2, v3, v4;
          if (face == 0) {
            v1 = get_vertex(mesh_left, mesh_up, mesh_front);
            v2 = get_vertex(mesh_left, mesh_up, mesh_back);
            v3 = get_vertex(mesh_right, mesh_up, mesh_back);
            v4 = get_vertex(mesh_right, mesh_up, mesh_front);
          }
          else if (face == 1) {
            v1 = get_vertex(mesh_left, mesh_up, mesh_back);
            v2 = get_vertex(mesh_left, mesh_up, mesh_front);
            v3 = get_vertex(mesh_right, mesh_up, mesh_front);
            v4 = get_vertex(mesh_right, mesh_up, mesh_back);
          }
          else if (face == 2) {
            v1 = get_vertex(mesh_up, mesh_front, mesh_left);
            v2 = get_vertex(mesh_up, mesh_back, mesh_left);
            v3 = get_vertex(mesh_up, mesh_back, mesh_right);
            v4 = get_vertex(mesh_up, mesh_front, mesh_right);
          }
          else if (face == 3) {
            v1 = get_vertex(mesh_up, mesh_back, mesh_left);
            v2 = get_vertex(mesh_up, mesh_front, mesh_left);
            v3 = get_vertex(mesh_up, mesh_front, mesh_right);
            v4 = get_vertex(mesh_up, mesh_back, mesh_right);
          }
          else if (face == 4) {
            v1 = get_vertex(mesh_front, mesh_left, mesh_up);
            v2 = get_vertex(mesh_back, mesh_left, mesh_up);
            v3 = get_vertex(mesh_back, mesh_right, mesh_up);
            v4 = get_vertex(mesh_front, mesh_right, mesh_up);
          }
          else if (face == 5) {
            v1 = get_vertex(mesh_back, mesh_left, mesh_up);
            v2 = get_vertex(mesh_front, mesh_left, mesh_up);
            v3 = get_vertex(mesh_front, mesh_right, mesh_up);
            v4 = get_vertex(mesh_back, mesh_right, mesh_up);
          }

          vertices->insert(vertices->end(), { v1, v2, v3, v3, v4, v1 });
        }
      }
    }
  }
};

#endif