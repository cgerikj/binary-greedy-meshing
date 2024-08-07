#ifndef CHUNK_RENDERER
#define CHUNK_RENDERER

#include <vector>
#include <utility>
#include <glad/glad.h>

static const int BUFFER_SIZE = 5e8; // 500 mb
static const uint32_t QUAD_SIZE = 8;
static const int MAX_DRAW_COMMANDS = 100000;

struct BufferSlot {
  uint32_t startByte;
  uint32_t sizeBytes;
};

struct DrawElementsIndirectCommand {
  uint32_t indexCount;    // (count) Quad count * 6
  uint32_t instanceCount; // 1
  uint32_t firstIndex;    // 0
  uint32_t baseQuad;      // (baseVertex) Starting index in the SSBO
  uint32_t baseInstance;  // Chunk x, y z, face index
};

struct BufferFit {
  uint32_t pos;
  uint32_t space;
  std::vector<BufferSlot*>::iterator iter;
};

class ChunkRenderer {
public:
  void init() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &commandBuffer);
    glGenBuffers(1, &SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, BUFFER_SIZE, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glGenBuffers(1, &IBO);
    int maxQuads = CS * CS * CS * 6;
    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < maxQuads; i++) {
      indices.push_back((i << 2) | 2u);
      indices.push_back((i << 2) | 0u);
      indices.push_back((i << 2) | 1u);
      indices.push_back((i << 2) | 1u);
      indices.push_back((i << 2) | 3u);
      indices.push_back((i << 2) | 2u);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size(), indices.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, MAX_DRAW_COMMANDS * sizeof(DrawElementsIndirectCommand), nullptr, GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
  };

  // TODO: Deallocate buffers
  ~ChunkRenderer() { };

  DrawElementsIndirectCommand* getDrawCommand(int quadCount, uint32_t baseInstance) {
    unsigned int requestedSize = quadCount * QUAD_SIZE;

    // Allocate at the end if possible
    if ((BUFFER_SIZE - allocationEnd) > requestedSize) {
      auto slot = new BufferSlot({ allocationEnd, requestedSize });
      usedSlots.push_back(slot);
      allocationEnd += requestedSize;
      return createCommand(*slot, baseInstance);
    }

    // Otherwise, iterate through and find gaps to allocate in
    // Slow!

    auto prev = usedSlots.begin();
    BufferFit bestFit;
    bool foundFit = false;

    unsigned int spaceInFront = usedSlots.front()->startByte;
    if (spaceInFront >= requestedSize) {
      foundFit = true;
      bestFit = BufferFit({
        0,
        spaceInFront,
        usedSlots.begin()
      });
    }

    if (usedSlots.size() == 1) {
      auto slot = new BufferSlot({ usedSlots.back()->startByte + usedSlots.back()->sizeBytes, requestedSize });
      usedSlots.insert(usedSlots.end(), slot);
      return createCommand(*slot, baseInstance);
    }

    for (auto it = usedSlots.begin() + 1; it != usedSlots.end(); ++it) {
      unsigned int pos = (*prev)->startByte + (*prev)->sizeBytes;
      unsigned int spaceBetween = (*it)->startByte - pos;
      if (spaceBetween >= requestedSize && (!foundFit || spaceBetween < bestFit.space)) {
        foundFit = true;
        bestFit = BufferFit({
          pos,
          spaceBetween,
          it
        });
      }
      prev = it;
    }

    if (foundFit) {
      auto slot = new BufferSlot({ bestFit.pos, requestedSize });
      usedSlots.insert(bestFit.iter, slot);
      return createCommand(*slot, baseInstance);
    }

    auto slot = new BufferSlot({ usedSlots.back()->startByte + usedSlots.back()->sizeBytes, requestedSize });
    usedSlots.insert(usedSlots.end(), slot);
    return createCommand(*slot, baseInstance);
  };

  void removeDrawCommand(const DrawElementsIndirectCommand* command) {
    for (auto it = begin(usedSlots); it != end(usedSlots); ++it) {
      if ((*it)->startByte == (command->baseQuad >> 2) * QUAD_SIZE) {
        usedSlots.erase(it);
        return;
      }
    }
  }

  void buffer(const DrawElementsIndirectCommand& command, void* vertices) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
    glBufferSubData(
      GL_SHADER_STORAGE_BUFFER,
      (command.baseQuad >> 2) * QUAD_SIZE,
      (command.indexCount / 6) * QUAD_SIZE,
      vertices
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  inline void addDrawCommand(const DrawElementsIndirectCommand& command) {
    drawCommands.insert(drawCommands.end(), command);
  };

  void render(Shader& shader, Camera& camera) {
    int numCommands = drawCommands.size();

    if (numCommands == 0) {
      return;
    }

    shader.use();
    shader.setMat4("u_projection", camera.projection);
    shader.setMat4("u_view", camera.getViewMatrix());
    shader.setVec3("eye_position", camera.position);

    glm::ivec3 intCamPosition = glm::ivec3(floor(camera.position));
    shader.setIVec3("eye_position_int", intCamPosition);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, numCommands * sizeof(DrawElementsIndirectCommand), drawCommands.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);

    glMultiDrawElementsIndirect(
      GL_TRIANGLES,
      GL_UNSIGNED_INT,
      (void*)0,
      numCommands,
      0
    );

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);

    // clear but don't deallocate
    drawCommands.clear();
  };

private:
  DrawElementsIndirectCommand* createCommand(BufferSlot& slot, uint32_t baseInstance) {
    return new DrawElementsIndirectCommand({
      (slot.sizeBytes / QUAD_SIZE) * 6,
      1,
      0,
      (slot.startByte / QUAD_SIZE) << 2,
      baseInstance
    });
  }

  unsigned int VAO = 0;
  unsigned int IBO = 0;
  unsigned int SSBO = 0;
  unsigned int commandBuffer = 0;

  std::vector<BufferSlot*> usedSlots;
  std::vector<DrawElementsIndirectCommand> drawCommands;
  uint32_t allocationEnd = 0;
};

#endif