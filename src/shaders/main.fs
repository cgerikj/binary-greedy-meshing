#version 460 core

layout(location=0) out vec3 out_color;

in VS_OUT {
  vec3 pos;
  flat vec3 normal;
  flat vec3 color;
} fs_in;

uniform vec3 eye_position;

const vec3 diffuse_color = vec3(0.15, 0.15, 0.15);
const vec3 rim_color = vec3(0.04, 0.04, 0.04);
const vec3 sun_position = vec3(250.0, 1000.0, 750.0);

void main() {
  vec3 L = normalize(sun_position - fs_in.pos);
  vec3 V = normalize(eye_position - fs_in.pos);

  float rim = 1 - max(dot(V, fs_in.normal), 0.0);
  rim = smoothstep(0.6, 1.0, rim);

  out_color = 
    fs_in.color +
    (diffuse_color * max(0, dot(L, fs_in.normal))) +
    (rim_color * vec3(rim, rim, rim))
  ;
}