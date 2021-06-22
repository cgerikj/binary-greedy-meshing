#version 430 core

layout (location = 0) in uint data;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
  float x = float(data&31);
  float y = float((data >> 5)&31);
  float z = float((data >> 10)&31);
  
  gl_Position = u_projection * u_view * vec4(x, y, z, 1);
} 