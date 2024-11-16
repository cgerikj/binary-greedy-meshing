#ifndef RLE_H
#define RLE_H

#include <vector>
#include <cstring>

inline void addRleRun(std::vector<uint8_t>& rleVoxels, uint8_t type, uint32_t length) {
  uint8_t subLength = 0;
  if (length <= 255) {
    subLength = length;
    length = 0;
  }
  else {
    subLength = 255;
    length -= 255;
  }

  rleVoxels.insert(rleVoxels.end(), { type, subLength });

  if (length > 0) {
    addRleRun(rleVoxels, type, length);
  }
}

namespace rle {
  void compress(std::vector<uint8_t> &voxels, std::vector<uint8_t> &rleVoxels) {
    uint8_t type = 0;
    uint32_t length = 0;

    for (const uint8_t& iType : voxels) {
      if (type == iType) {
        length++;
      }
      else {
        addRleRun(rleVoxels, type, length);

        type = iType;
        length = 1;
      }
    }

    addRleRun(rleVoxels, (uint8_t)type, length);
  }

  inline const uint64_t getBitRange(uint8_t low, uint8_t high) {
    return  ((1ULL << (high - low + 1)) - 1) << low;
  }

  void decompressToVoxelsAndOpaqueMask(uint8_t* rleVoxels, int rleSize, uint8_t* voxels, uint64_t* opaqueMask) {
    uint8_t* p = rleVoxels;
    uint8_t* p_end = rleVoxels + rleSize;
    uint8_t* u_p = voxels;

    int opaqueMaskIndex = 0;
    int opaqueMaskBitIndex = 0;

    while (p != p_end) {
      uint8_t type = *p;
      uint8_t len = *(p + 1);

      std::memset(u_p, type, len);

      // Decompress into opaqueMask
      int remainingLength = len;
      while (remainingLength) {
        int remainingBitsInIndex = 64 - opaqueMaskBitIndex;
        // Partial same int
        if (remainingLength < remainingBitsInIndex) {
          if (type) {
            opaqueMask[opaqueMaskIndex] |= getBitRange(opaqueMaskBitIndex, opaqueMaskBitIndex + remainingLength - 1);
          }
          opaqueMaskBitIndex += remainingLength;
          remainingLength = 0;
        }
        // Set n integers
        else if (remainingLength >= 64 && opaqueMaskBitIndex == 0) {
          int count = std::floor(remainingLength / 64);
          if (type) {
            memset(&opaqueMask[opaqueMaskIndex], (uint64_t)-1, count * sizeof(uint64_t));
          }
          opaqueMaskIndex += count;
          remainingLength -= count * 64;
        }
        // Rest of int + left over
        else if (remainingLength >= remainingBitsInIndex) {
          if (type) {
            opaqueMask[opaqueMaskIndex] |= getBitRange(opaqueMaskBitIndex, 63);
          }
          remainingLength -= remainingBitsInIndex;
          opaqueMaskIndex++;
          opaqueMaskBitIndex = 0;
        }
      }

      u_p += len;
      p += 2;
    }
  }
};

#endif
