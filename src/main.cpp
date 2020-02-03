#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <algorithm>

#include "mesher.h"
#include "shader.h"
#include "camera.h"
#include "skybox.h"
#include "noise.h"
#include "light.h"

void create_chunk();

GLFWwindow* init_window() {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 4);

  GLFWwindow* window = glfwCreateWindow(1280, 720, "Binary Greedy Meshing", nullptr, nullptr);
  if (!window) {
    fprintf(stderr, "Unable to create GLFW window\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return nullptr;
  }

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwMakeContextCurrent(window);

  if (!gladLoadGL()) {
    fprintf(stderr, "Unable to initialize glad\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return nullptr;
  }

  return window;
}

void GLAPIENTRY message_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam
) {
  std::string SEVERITY = "";
  switch (severity) {
  case GL_DEBUG_SEVERITY_LOW:
    SEVERITY = "LOW";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    SEVERITY = "MEDIUM";
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    SEVERITY = "HIGH";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    SEVERITY = "NOTIFICATION";
    break;
  }
  fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = %s, message = %s\n",
    type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "",
    type, SEVERITY, message);
}

bool init_opengl() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
  glDebugMessageCallback(message_callback, 0);

  glEnable(GL_DEPTH_TEST);

  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  glClearColor(0.529f, 0.808f, 0.922f, 0.0f);

  glEnable(GL_MULTISAMPLE);

  return true;
};

void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

Shader* shader = nullptr;
Shader* skyboxShader = nullptr;
Camera* camera = nullptr;
Noise noise;

float last_x = 0.0f;
float last_y = 0.0f;

enum class MESH_TYPE: int {
  TERRAIN,
  RANDOM,
  Count
};

int mesh_type = (int)MESH_TYPE::TERRAIN;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
  camera->processMouseMovement(xpos - last_x, last_y - ypos);
  last_x = xpos;
  last_y = ypos;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, true);
  }

  else if (key == GLFW_KEY_X && action == GLFW_RELEASE) {
    GLint lastPolyMode[2];
    glGetIntegerv(GL_POLYGON_MODE, lastPolyMode);
    if (lastPolyMode[0] == GL_FILL) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  }

  else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
    create_chunk();
  }

  else if (key == GLFW_KEY_TAB && action == GLFW_RELEASE) {
    mesh_type++;
    if (mesh_type >= (int)MESH_TYPE::Count) {
      mesh_type = 0;
    }
    create_chunk();
  }
}

struct Attribute {
  GLuint type;
  GLuint type_size;
  GLuint length;
  bool int_type;
};

GLuint VAO, VBO;
int vertexCount = 0;

void create_chunk() {
  std::vector<uint8_t> voxels(CS_P3);
  std::fill(voxels.begin(), voxels.end(), 0);

  switch (mesh_type) {
    case MESH_TYPE::TERRAIN: {
      noise.generateTerrain(voxels, std::rand());
      break;
    }

    case MESH_TYPE::RANDOM: {
      noise.generateWhiteNoiseTerrain(voxels, std::rand());
      break;
    }
  }

  std::vector<uint8_t> light_map(CS_P3);
  std::fill(light_map.begin(), light_map.end(), 0);
  calculate_light(voxels, light_map);

  auto vertices = mesh(voxels, light_map, glm::ivec3(0));
  if (vertices == nullptr) {
    vertexCount = 0;
  } else {
    vertexCount = vertices->size();

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices->data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    vertices->clear();
    vertices->shrink_to_fit();
    delete vertices;
  }

  printf("vertex count: %i\n", vertexCount);
}

int main(int argc, char* argv[]) {
  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit()) {
    fprintf(stderr, "Unable to initialize GLFW\n");
    return 1;
  }

  auto window = init_window();
  if (!window) {
    return 1;
  }
  glfwSetWindowPos(window, 0, 31);
  glfwSwapInterval(1);

  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetKeyCallback(window, key_callback);

  if (!init_opengl()) {
    fprintf(stderr, "Unable to initialize glad/opengl\n");
    return 1;
  }  

  std::vector<Attribute> attributes = {
    { GL_SHORT, sizeof(GLshort), 3, false },
    { GL_UNSIGNED_BYTE, sizeof(GLubyte), 1, true },
    { GL_UNSIGNED_BYTE, sizeof(GLubyte), 1, true },
    { GL_UNSIGNED_BYTE, sizeof(GLubyte), 1, true }
  };

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindVertexArray(VAO);
  for (unsigned int i = 0; i < attributes.size(); ++i) {
    glEnableVertexAttribArray(i);
  }
  unsigned int offset = 0;
  int i = 0;
  for (auto attr : attributes) {
    if (attr.int_type) {
      glVertexAttribIPointer(i, attr.length, attr.type, sizeof(Vertex), (void*)offset);
    }
    else {
      glVertexAttribPointer(i, attr.length, attr.type, false, sizeof(Vertex), (void*)offset);
    }
    offset += attr.type_size * attr.length;
    i++;
  }
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  create_chunk();

  shader = new Shader("main", "main");
  skyboxShader = new Shader("skybox", "skybox");
  camera = new Camera(glm::vec3(31, 65, -5));
  camera->handleResolution(1280, 720);

  Skybox skybox;

  srand(time(NULL));

  float forwardMove = 0.0f;
  float rightMove = 0.0f;
  float noclipSpeed = .0f;

  float deltaTime = 0.0f;

  auto lastFrame = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forwardMove = 1.0f;
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forwardMove = -1.0f;
    else forwardMove = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) rightMove = 1.0f;
    else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) rightMove = -1.0f;
    else rightMove = 0.0f;
    auto wishdir = (camera->front * forwardMove) + (camera->right * rightMove);
    camera->position += noclipSpeed * wishdir * deltaTime;

    skyboxShader->use();
    skyboxShader->setMat4("u_projection", camera->projection);
    skyboxShader->setMat4("u_view", camera->getViewMatrix());
    auto skybox_viewmatrix = glm::mat4(glm::mat3(camera->getViewMatrix()));
    skybox.render(*skyboxShader, skybox_viewmatrix);

    if (vertexCount > 0) {
      shader->use();
      shader->setMat4("u_projection", camera->projection);
      shader->setMat4("u_view", camera->getViewMatrix());
      shader->setVec3("eye_position", camera->position);
      glBindVertexArray(VAO);
      glDrawArrays(GL_TRIANGLES, 0, vertexCount);
      glBindVertexArray(0);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return 0;
}