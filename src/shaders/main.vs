#version 430 core

layout (location = 0) in uvec2 data;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec4 frag_viewspace;
out vec3 frag_normal;
out vec3 frag_pos;
flat out float frag_light;
flat out uint frag_type;

uniform vec3 NORMALS[6] = {
  vec3( 0, 1, 0 ),
  vec3(0, -1, 0 ),
  vec3( 1, 0, 0 ),
  vec3( -1, 0, 0 ),
  vec3( 0, 0, 1 ),
  vec3( 0, 0, -1 )
};

void main() {
  float x = float(data[0]&(63));
  float y = float((data[0] >> 14)&(63));
  uint type = (data[0] >> 24)&(63);

  float z = float(data[1]&(63));
  uint light = (data[1] >> 14)&(63);
  uint norm = (data[1] >> 22)&(63);

  frag_pos = vec3(x, y, z) - vec3(0.5);
  frag_viewspace = u_view * vec4(frag_pos, 1);
  frag_normal = NORMALS[norm];
  frag_light = float(light) / 32.0;
  frag_type = type;
  
  gl_Position = u_projection * frag_viewspace;
}