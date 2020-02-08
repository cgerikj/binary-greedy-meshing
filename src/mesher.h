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

inline const bool compare_ao(std::vector<uint8_t>& voxels, int axis, int forward, int right, int c, int forward_offset, int right_offset) {
  // Forward
  if (voxels.at(get_axis_i(axis, right, forward - 1, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset, forward + forward_offset - 1, c)) > 0) return false;

  // Back
  if (voxels.at(get_axis_i(axis, right, forward + 1, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset, forward + forward_offset + 1, c)) > 0) return false;

  // Left
  if (voxels.at(get_axis_i(axis, right - 1, forward, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset - 1, forward + forward_offset, c)) > 0) return false;

  // Right
  if (voxels.at(get_axis_i(axis, right + 1, forward, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset + 1, forward + forward_offset, c)) > 0) return false;

  // Left-forward
  if (voxels.at(get_axis_i(axis, right - 1, forward - 1, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset - 1, forward + forward_offset - 1, c)) > 0) return false;

  // Left-back
  if (voxels.at(get_axis_i(axis, right - 1, forward + 1, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset - 1, forward + forward_offset + 1, c)) > 0) return false;

  // Right-forward
  if (voxels.at(get_axis_i(axis, right + 1, forward - 1, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset + 1, forward + forward_offset - 1, c)) > 0) return false;

  // Right-back
  if (voxels.at(get_axis_i(axis, right + 1, forward + 1, c)) > 0 !=
      voxels.at(get_axis_i(axis, right + right_offset + 1, forward + forward_offset + 1, c)) > 0) return false;

  return true;
}

inline const bool compare_forward(std::vector<uint8_t>& voxels, std::vector<uint8_t>& light_map, int axis, int forward, int right, int bit_pos, int light_dir) {
  return
    voxels.at(get_axis_i(axis, right, forward, bit_pos)) == voxels.at(get_axis_i(axis, right, forward + 1, bit_pos)) &&
    light_map.at(get_axis_i(axis, right, forward, bit_pos + light_dir)) == light_map.at(get_axis_i(axis, right, forward + 1, bit_pos + light_dir)) &&
    compare_ao(voxels, axis, forward, right, bit_pos + light_dir, 1, 0)
  ;
}

inline const bool compare_right(std::vector<uint8_t>& voxels, std::vector<uint8_t>& light_map, int axis, int forward, int right, int bit_pos, int light_dir) {
  return
    voxels.at(get_axis_i(axis, right, forward, bit_pos)) == voxels.at(get_axis_i(axis, right + 1, forward, bit_pos)) &&
    light_map.at(get_axis_i(axis, right, forward, bit_pos + light_dir)) == light_map.at(get_axis_i(axis, right + 1, forward, bit_pos + light_dir)) &&
    compare_ao(voxels, axis, forward, right, bit_pos + light_dir, 0, 1)
  ;
}

inline const void insert_quad(std::vector<uint32_t>* vertices, uint32_t x, uint32_t y, uint32_t z, uint32_t type, uint32_t light, uint32_t norm, uint32_t ao) {
  vertices->insert(vertices->end(),
    {
      (ao << 30) | (norm << 27) | (light << 23) | (type << 18) | (z << 12) | (y << 6) | x
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
    int axis = face / 2;
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

          if (bit_pos == 0 || bit_pos == CS_P - 1) continue;

          if (compare_forward(voxels, light_map, axis, forward, right, bit_pos, light_dir)) {
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
            compare_right(voxels, light_map, axis, forward, right, bit_pos, light_dir)
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

          uint8_t type = voxels.at(get_axis_i(axis, right, forward, bit_pos));
          uint8_t light = light_map.at(get_axis_i(axis, right, forward, bit_pos + light_dir));

          int c = bit_pos + light_dir;
          uint8_t ao_F = (voxels.at(get_axis_i(axis, right, forward - 1, c)) > 0) ? 1 : 0;
          uint8_t ao_B = (voxels.at(get_axis_i(axis, right, forward + 1, c)) > 0) ? 1 : 0;
          uint8_t ao_L = (voxels.at(get_axis_i(axis, right - 1, forward, c)) > 0) ? 1 : 0;
          uint8_t ao_R = (voxels.at(get_axis_i(axis, right + 1, forward, c)) > 0) ? 1 : 0;

          uint8_t ao_LFC = (voxels.at(get_axis_i(axis, right - 1, forward - 1, c)) > 0) ? 1 : 0;
          uint8_t ao_LBC = (voxels.at(get_axis_i(axis, right - 1, forward + 1, c)) > 0) ? 1 : 0;
          uint8_t ao_RFC = (voxels.at(get_axis_i(axis, right + 1, forward - 1, c)) > 0) ? 1 : 0;
          uint8_t ao_RBC = (voxels.at(get_axis_i(axis, right + 1, forward + 1, c)) > 0) ? 1 : 0;

          uint8_t ao_LB = ao_L + ao_B + ao_LBC;
          uint8_t ao_LF = ao_L + ao_F + ao_LFC;
          uint8_t ao_RB = ao_R + ao_B + ao_RBC;
          uint8_t ao_RF = ao_R + ao_F + ao_RFC;

          merged_forward[(right * CS_P) + bit_pos] = 0;
          merged_right[bit_pos] = 0;

          if (face == 0) {
            insert_quad(vertices, mesh_left,  mesh_up, mesh_front, type, light, face, ao_LF);
            insert_quad(vertices, mesh_left,  mesh_up, mesh_back,  type, light, face, ao_LB);
            insert_quad(vertices, mesh_right, mesh_up, mesh_back,  type, light, face, ao_RB);
            insert_quad(vertices, mesh_right, mesh_up, mesh_back,  type, light, face, ao_RB);
            insert_quad(vertices, mesh_right, mesh_up, mesh_front, type, light, face, ao_RF);
            insert_quad(vertices, mesh_left,  mesh_up, mesh_front, type, light, face, ao_LF);
          }
          else if (face == 1) {
            insert_quad(vertices, mesh_left,  mesh_up, mesh_back,  type, light, face, ao_LB);
            insert_quad(vertices, mesh_left,  mesh_up, mesh_front, type, light, face, ao_LF);
            insert_quad(vertices, mesh_right, mesh_up, mesh_front, type, light, face, ao_RF);
            insert_quad(vertices, mesh_right, mesh_up, mesh_front, type, light, face, ao_RF);
            insert_quad(vertices, mesh_right, mesh_up, mesh_back,  type, light, face, ao_RB);
            insert_quad(vertices, mesh_left,  mesh_up, mesh_back,  type, light, face, ao_LB);
          }
          else if (face == 2) {
            insert_quad(vertices, mesh_up, mesh_front, mesh_left,  type, light, face, ao_LF);
            insert_quad(vertices, mesh_up, mesh_back,  mesh_left,  type, light, face, ao_LB);
            insert_quad(vertices, mesh_up, mesh_back,  mesh_right, type, light, face, ao_RB);
            insert_quad(vertices, mesh_up, mesh_back,  mesh_right, type, light, face, ao_RB);
            insert_quad(vertices, mesh_up, mesh_front, mesh_right, type, light, face, ao_RF);
            insert_quad(vertices, mesh_up, mesh_front, mesh_left,  type, light, face, ao_LF);
          }
          else if (face == 3) {
            insert_quad(vertices, mesh_up, mesh_back,  mesh_left,  type, light, face, ao_LB);
            insert_quad(vertices, mesh_up, mesh_front, mesh_left,  type, light, face, ao_LF);
            insert_quad(vertices, mesh_up, mesh_front, mesh_right, type, light, face, ao_RF);
            insert_quad(vertices, mesh_up, mesh_front, mesh_right, type, light, face, ao_RF);
            insert_quad(vertices, mesh_up, mesh_back,  mesh_right, type, light, face, ao_RB);
            insert_quad(vertices, mesh_up, mesh_back,  mesh_left,  type, light, face, ao_LB);
          }
          else if (face == 4) {
            insert_quad(vertices, mesh_front, mesh_left,  mesh_up, type, light, face, ao_LF);
            insert_quad(vertices, mesh_back,  mesh_left,  mesh_up, type, light, face, ao_LB);
            insert_quad(vertices, mesh_back,  mesh_right, mesh_up, type, light, face, ao_RB);
            insert_quad(vertices, mesh_back,  mesh_right, mesh_up, type, light, face, ao_RB);
            insert_quad(vertices, mesh_front, mesh_right, mesh_up, type, light, face, ao_RF);
            insert_quad(vertices, mesh_front, mesh_left,  mesh_up, type, light, face, ao_LF);
          }
          else if (face == 5) {
            insert_quad(vertices, mesh_back,  mesh_left,  mesh_up, type, light, face, ao_LB);
            insert_quad(vertices, mesh_front, mesh_left,  mesh_up, type, light, face, ao_LF);
            insert_quad(vertices, mesh_front, mesh_right, mesh_up, type, light, face, ao_RF);
            insert_quad(vertices, mesh_front, mesh_right, mesh_up, type, light, face, ao_RF);
            insert_quad(vertices, mesh_back,  mesh_right, mesh_up, type, light, face, ao_RB);
            insert_quad(vertices, mesh_back,  mesh_left,  mesh_up, type, light, face, ao_LB);
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