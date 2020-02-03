#version 430 core
out vec4 frag_color;

in float y_pos;

const float start = 0.45;
const float stop = 1.0;

vec3 fog_color = vec3(0.75, 0.75, 0.75);
vec3 sky_color = vec3(0.5, 0.7, 1.0);

void main() {
  if (y_pos < start) {
    frag_color = vec4(fog_color, 1.0);
  } else if (y_pos < stop) {
    float ratio = (stop - y_pos) / (stop - start);
    ratio = smoothstep(0.5, 1.0, ratio);
    frag_color = vec4(mix(sky_color, fog_color, ratio), 1.0);
  } else {
    frag_color = vec4(sky_color, 1.0);
  }
}