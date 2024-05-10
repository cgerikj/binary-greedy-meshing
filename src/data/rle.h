#ifndef RLE_H
#define RLE_H

#include <vector>

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

  void decompress(std::vector<uint8_t> &rleVoxels, std::vector<uint8_t> &voxels) {
    uint8_t* p = rleVoxels.data();
    uint8_t* p_end = rleVoxels.data() + rleVoxels.size();
    uint8_t* u_p = voxels.data();
    while (p != p_end) {
      uint8_t len = *(p + 1);

      std::memset(u_p, *p, len);

      u_p += len;
      p += 2;
    }
  }
};

#endif