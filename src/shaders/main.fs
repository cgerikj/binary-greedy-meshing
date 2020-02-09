#version 430 core
out vec4 frag_color;

in vec4 frag_viewspace;
in vec3 frag_pos;
in vec3 frag_normal;
in float frag_ao;
flat in float frag_light;
flat in uint frag_type;

uniform vec3 eye_position;

const vec3 diffuse_color = vec3(0.15, 0.15, 0.15);
const vec3 rim_color = vec3(0.04, 0.04, 0.04);
const vec3 sun_position = vec3(250.0, 1000.0, 750.0);

void main() {
  vec3 final_color;
  if (frag_type == 1) {
   final_color = vec3(0.6, 0.1, 0.1);
  }
  else if (frag_type == 2) {
   final_color = vec3(0.1, 0.6, 0.1);
  }
  else {
   final_color = vec3(0.1, 0.1, 0.6);
  }

  vec3 L = normalize(sun_position - frag_pos);
  vec3 V = normalize(eye_position - frag_pos);

  final_color += diffuse_color * max(0, dot(L, frag_normal));

  float rim = 1 - max(dot(V, frag_normal), 0.0);
  rim = smoothstep(0.6, 1.0, rim);
  final_color += rim_color * vec3(rim, rim, rim);

  final_color *= frag_light * smoothstep(0.0, 1.0, frag_ao);

  frag_color = vec4(final_color, 1.0);
}