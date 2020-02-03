#version 430 core

layout (location = 0) in vec3 in_pos;

uniform mat4 u_view;
uniform mat4 u_projection;

out float y_pos;

void main() {
  y_pos = in_pos.y + 0.5;
  vec4 pos = u_projection * u_view * vec4(in_pos, 1);
  gl_Position = pos.xyww;
}