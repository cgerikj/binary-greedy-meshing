#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <GLFW/glfw3.h>

const float YAW         =  0.0f;
const float PITCH       =  0.0f;
const float SENSITIVITY =  0.075f;
const float FOV         =  80.0f;

class Camera {
public:
  glm::vec3 position;
  glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 right;
  glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4 projection;
  float yaw = YAW;
  float pitch = PITCH;
  float mouseSensitivity = SENSITIVITY;
  float fov = FOV;
  float nearD = 1.0f;
  float farD = 10000.0f;
  float ratio;

  Camera(glm::vec3 position) : position(position) {
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    handleResolution(mode->width, mode->height);
    updateCameraVectors();
  }

  void handleResolution(int width, int height) {
    ratio = (float)width / (float)height;

    projection = glm::perspective(glm::radians(fov), ratio, nearD, farD);
  }

  void updatePosition(glm::vec3 pos) {
    position = pos;
  }

  glm::mat4 getViewMatrix() {
    return glm::lookAt(position, position + front, up);
  }

  void processMouseMovement(float xOffset, float yOffset) {
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    yaw   += xOffset;
    pitch += yOffset;

    if (pitch > 89.9f) pitch = 89.9f;
    if (pitch < -89.9f) pitch = -89.9f;

    updateCameraVectors();
  }

private:
  void updateCameraVectors() {
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
  }
};

#endif