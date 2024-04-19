#version 430 core
out vec4 frag_color;

in vec4 frag_viewspace;
in vec3 frag_pos;
in vec3 frag_normal;
in float frag_ao;
flat in uint frag_type;

uniform vec3 eye_position;

const vec3 diffuse_color = vec3(0.15, 0.15, 0.15);
const vec3 rim_color = vec3(0.04, 0.04, 0.04);
const vec3 sun_position = vec3(250.0, 1000.0, 750.0);

vec3 get_color(uint frag_type) {
  switch (frag_type) {
    case 1: return vec3(0.6, 0.1, 0.1);
    case 2: return vec3(0.1, 0.6, 0.1);
    case 3: return vec3(0.1, 0.1, 0.6);
    case 4: return vec3(0.1, 0.6, 0.6);
    case 5: return vec3(0.6, 0.1, 0.6);
    case 6: return vec3(0.6, 0.6, 0.1);
  }
}

void main() {
  vec3 final_color = get_color(frag_type);

  vec3 L = normalize(sun_position - frag_pos);
  vec3 V = normalize(eye_position - frag_pos);

  final_color += diffuse_color * max(0, dot(L, frag_normal));

  float rim = 1 - max(dot(V, frag_normal), 0.0);
  rim = smoothstep(0.6, 1.0, rim);
  final_color += rim_color * vec3(rim, rim, rim);

  float ao = clamp(frag_ao, 0.0, 1.0);
  final_color *= smoothstep(0.0, 1.0, ao);

  frag_color = vec4(final_color, 1.0);
}