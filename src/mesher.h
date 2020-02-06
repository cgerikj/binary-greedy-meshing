#ifndef MESHER_H
#define MESHER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <stdint.h>
#include "timer.h"
#include "constants.h"

struct Mesh {
  std::vector<uint32_t>* vertices;
  std::vector<uint32_t>* indices;
};

// MSVC specific ctz
// Use __builtin_ctzll for gcc
inline const int CTZ(uint64_t &x) {
  unsigned long index;
  _BitScanForward64(&index, x);
  return (int)index;
}

inline const int get_axis_i(const int &axis, const int &a, const int &b, const int &c) {
  if (axis == 0) return b + (a * CS_P) + (c * CS_P2);
  else if (axis == 1) return a + (c * CS_P) + (b* CS_P2);
  else return c + (b * CS_P) + (a * CS_P2);
}

inline const void insert_quad(std::vector<uint32_t>* vertices, uint32_t x, uint32_t y, uint32_t z, uint32_t type, uint32_t light, uint32_t norm) {
  vertices->insert(vertices->end(),
    {
      (norm << 27) | (light << 23) | (type << 18) | (z << 12) | (y << 6) | x
    }
   );
}

// voxels - 64^3 (includes neighboring voxels)
// light_map - ^
std::vector<uint32_t>* mesh(std::vector<uint8_t>& voxels, std::vector<uint8_t>& light_map) {
  Timer timer("meshing", true);

  uint64_t axis_cols[CS_P2 * 3] = { 0 };
  uint64_t col_face_masks[CS_P2 * 6];

  auto vertices = new std::vector<uint32_t>();

  // Step 1: Convert to binary representation for each direction
  auto p = voxels.begin();
  for (int y = 0; y < CS_P; y++) {
    for (int x = 0; x < CS_P; x++) {
      uint64_t zb = 0;
      for (int z = 0; z < CS_P; z++) {
        if (*p > 0) {
          axis_cols[x + (z * CS_P)] |= 1ULL << y;
          axis_cols[z + (y * CS_P) + (CS_P2)] |= 1ULL << x;
          zb |= 1ULL << z;
        }
        p++;
      }
      axis_cols[y + (x * CS_P) + (CS_P2 * 2)] = zb;
    }
  }

  // Step 2: Visible face culling
  for (int axis = 0; axis <= 2; axis++) {
    for (int i = 0; i < CS_P2; i++) {
      uint64_t col = axis_cols[(CS_P2 * axis) + i];
      col_face_masks[(CS_P2 * (axis * 2)) + i] = col & ~((col >> 1) | (1ULL << CS_P - 1));
      col_face_masks[(CS_P2 * (axis * 2 + 1)) + i] = col & ~((col << 1) | 1ULL);
    }
  }

  // Step 3: Greedy meshing
  for (int face = 0; face < 6; face++) {
    int light_dir = face % 2 == 0 ? 1 : -1;
    int merged_forward[CS_P2] = { 0 };
    for (int forward = 1; forward < CS_P - 1; forward++) {
      uint64_t bits_walking_right = 0;
      int merged_right[CS_P] = { 0 };
      for (int right = 1; right < CS_P - 1; right++) {
        uint64_t bits_here = col_face_masks[right + (forward * CS_P) + (face * CS_P2)];
        uint64_t bits_forward = forward >= CS ? 0 : col_face_masks[right + (forward * CS_P) + (face * CS_P2) + CS_P];
        uint64_t bits_right = right >= CS ? 0 : col_face_masks[right + 1 + (forward * CS_P) + (face * CS_P2)];
        uint64_t bits_merging_forward = bits_here & bits_forward & ~bits_walking_right;
        uint64_t bits_merging_right = bits_here & bits_right;

        uint64_t copy_front = bits_merging_forward;
        while (copy_front) {
          int bit_pos = CTZ(copy_front);
          copy_front &= ~(1ULL << bit_pos);

          if (
            voxels.at(get_axis_i(face / 2, right, forward, bit_pos)) ==
            voxels.at(get_axis_i(face / 2, right, forward + 1, bit_pos)) &&
            light_map.at(get_axis_i(face / 2, right, forward, bit_pos + light_dir)) ==
            light_map.at(get_axis_i(face / 2, right, forward + 1, bit_pos + light_dir))
            ) {
            merged_forward[(right * CS_P) + bit_pos]++;
          }
          else {
            bits_merging_forward &= ~(1ULL << bit_pos);
          }
        }

        uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
        while (bits_stopped_forward) {
          int bit_pos = CTZ(bits_stopped_forward);
          bits_stopped_forward &= ~(1ULL << bit_pos);

          // Discards faces from neighbor voxels
          if (bit_pos == 0 || bit_pos == CS_P - 1) continue;

          if (
            (bits_merging_right & (1ULL << bit_pos)) != 0 &&
            merged_forward[(right * CS_P) + bit_pos] == merged_forward[(right + 1) * CS_P + bit_pos] &&
            voxels.at(get_axis_i(face / 2, right, forward, bit_pos)) ==
            voxels.at(get_axis_i(face / 2, right + 1, forward, bit_pos)) &&
            light_map.at(get_axis_i(face / 2, right, forward, bit_pos + light_dir)) ==
            light_map.at(get_axis_i(face / 2, right + 1, forward, bit_pos + light_dir))
            ) {
            bits_walking_right |= 1ULL << bit_pos;
            merged_right[bit_pos]++;
            merged_forward[(right * CS_P) + bit_pos] = 0;
            continue;
          }
          bits_walking_right &= ~(1ULL << bit_pos);

          uint8_t mesh_left = right - merged_right[bit_pos];
          uint8_t mesh_right = right + 1;
          uint8_t mesh_front = forward - merged_forward[(right * CS_P) + bit_pos];
          uint8_t mesh_back = forward + 1;
          uint8_t mesh_up = bit_pos + (face % 2 == 0 ? 1 : 0);

          uint8_t type = voxels.at(get_axis_i(face / 2, right, forward, bit_pos));
          uint8_t light = light_map.at(get_axis_i(face / 2, right, forward, bit_pos + light_dir));

          merged_forward[(right * CS_P) + bit_pos] = 0;
          merged_right[bit_pos] = 0;

          if (face == 0) {
            insert_quad(vertices, mesh_left, mesh_up, mesh_front, type, light, face);
            insert_quad(vertices, mesh_left,  mesh_up, mesh_back, type, light, face);
            insert_quad(vertices, mesh_right, mesh_up, mesh_back, type, light, face);
            insert_quad(vertices, mesh_right, mesh_up, mesh_back, type, light, face);
            insert_quad(vertices, mesh_right, mesh_up, mesh_front, type, light, face);
            insert_quad(vertices, mesh_left, mesh_up, mesh_front, type, light, face);
          }
          else if (face == 1) {
            insert_quad(vertices, mesh_left, mesh_up, mesh_back, type, light, face);
            insert_quad(vertices, mesh_left,  mesh_up, mesh_front, type, light, face);
            insert_quad(vertices, mesh_right, mesh_up, mesh_front, type, light, face);
            insert_quad(vertices, mesh_right, mesh_up, mesh_front, type, light, face);
            insert_quad(vertices, mesh_right, mesh_up, mesh_back, type, light, face);
            insert_quad(vertices, mesh_left, mesh_up, mesh_back, type, light, face);
          }
          else if (face == 2) {
            insert_quad(vertices, mesh_up, mesh_front, mesh_left, type, light, face);
            insert_quad(vertices, mesh_up, mesh_back, mesh_left, type, light, face);
            insert_quad(vertices, mesh_up, mesh_back, mesh_right, type, light, face);
            insert_quad(vertices, mesh_up, mesh_back, mesh_right, type, light, face);
            insert_quad(vertices, mesh_up, mesh_front, mesh_right, type, light, face);
            insert_quad(vertices, mesh_up, mesh_front, mesh_left, type, light, face);
          }
          else if (face == 3) {
            insert_quad(vertices, mesh_up, mesh_back, mesh_left, type, light, face);
            insert_quad(vertices, mesh_up, mesh_front, mesh_left, type, light, face);
            insert_quad(vertices, mesh_up, mesh_front, mesh_right, type, light, face);
            insert_quad(vertices, mesh_up, mesh_front, mesh_right, type, light, face);
            insert_quad(vertices, mesh_up, mesh_back, mesh_right, type, light, face);
            insert_quad(vertices, mesh_up, mesh_back, mesh_left, type, light, face);
          }
          else if (face == 4) {
            insert_quad(vertices, mesh_front, mesh_left, mesh_up, type, light, face);
            insert_quad(vertices, mesh_back, mesh_left, mesh_up, type, light, face);
            insert_quad(vertices, mesh_back, mesh_right, mesh_up, type, light, face);
            insert_quad(vertices, mesh_back, mesh_right, mesh_up, type, light, face);
            insert_quad(vertices, mesh_front,mesh_right,mesh_up, type, light, face);
            insert_quad(vertices, mesh_front, mesh_left, mesh_up, type, light, face);
          }
          else if (face == 5) {
            insert_quad(vertices, mesh_back, mesh_left, mesh_up, type, light, face);
            insert_quad(vertices, mesh_front, mesh_left, mesh_up, type, light, face);
            insert_quad(vertices, mesh_front, mesh_right, mesh_up, type, light, face);
            insert_quad(vertices, mesh_front, mesh_right, mesh_up, type, light, face);
            insert_quad(vertices, mesh_back, mesh_right, mesh_up, type, light, face);
            insert_quad(vertices, mesh_back, mesh_left, mesh_up, type, light, face);
          }
        }
      }
    }
  }

  size_t vertexLength = vertices->size();

  if (vertexLength == 0) {
    delete vertices;
    return nullptr;
  }

  return vertices;
};

#endif