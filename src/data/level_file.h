#ifndef LEVEL_FILE_H
#define LEVEL_FILE_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <stdlib.h>
#include <iterator>
#include <glm/glm.hpp>
#include "rle.h"

/*
* File structure:
* 1 byte: Square world size (0-255 chunks in x and z)
* size * size * sizeof(ChunkTableEntry) bytes: Lookup table of RLE compressed chunks
* The rest of the buffer contain the chunks. Indices in the table origate from byte 0 in the buffer.
*/

#if defined _WIN32 || defined __CYGWIN__
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

struct ChunkTableEntry {
  uint32_t key, rleDataBegin, rleDataSize;
};

class LevelFile {
public:
  std::vector<ChunkTableEntry> chunkTable;
  std::vector<uint8_t> buffer;

  void initialize(int s) {
    size = s;
    dataBufferHead = 1 + (size * size * sizeof(ChunkTableEntry));

    uint32_t maxFileSize = 1e5 * size * size;
    buffer.reserve(maxFileSize);
    buffer.assign(maxFileSize, 0);
  }

  uint8_t getSize() {
    return size;
  }

  void loadFromFile(std::string levelName) {
    std::ifstream file(".." PATH_SEP "levels" PATH_SEP + levelName, std::ios::binary);
    file.unsetf(std::ios::skipws);

    if (file.is_open()) {
      std::streampos fileSize;
      file.seekg(0, std::ios::end);
      fileSize = file.tellg();
      file.seekg(0, std::ios::beg);

      buffer.reserve(fileSize);
      buffer.insert(buffer.begin(), std::istream_iterator<uint8_t>(file), std::istream_iterator<uint8_t>());

      int bufferHead = 0;

      size = buffer[0];
      bufferHead++;

      uint32_t tableLength = size * size;
      uint32_t tableSize =  tableLength * sizeof(ChunkTableEntry);

      chunkTable.resize(tableLength);

      std::memcpy(chunkTable.data(), &buffer[bufferHead], tableSize);

      file.close();
    }
  }

  void compressAndAddChunk(std::vector<uint8_t>& voxels, uint32_t key) {
    auto rleVoxels = std::vector<uint8_t>();
    rle::compress(voxels, rleVoxels);

    chunkTable.push_back(ChunkTableEntry({
      key,
      dataBufferHead,
      (uint32_t)rleVoxels.size()
    }));

    memcpy(buffer.data() + dataBufferHead, rleVoxels.data(), rleVoxels.size());
    dataBufferHead += rleVoxels.size();
  }

  void saveToFile(std::string newLevelName) {
    buffer[0] = size;

    memcpy(buffer.data() + 1, chunkTable.data(), chunkTable.size() * sizeof(ChunkTableEntry));

    buffer.resize(dataBufferHead + 1);

    std::string saveDir = ".." PATH_SEP "levels" PATH_SEP;
    std::filesystem::create_directory(saveDir);

    std::ofstream file;
    file.open(saveDir + newLevelName, std::ios::out | std::ios::binary);
    file.write((char*)buffer.data(), dataBufferHead);
    file.close();
  }

private:
  uint8_t size = 0;
  uint32_t dataBufferHead = 1;
};

#endif
