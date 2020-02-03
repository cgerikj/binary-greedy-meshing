#version 430 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in uint in_type;
layout (location = 2) in uint in_light;
layout (location = 3) in uint in_normal;

uniform mat4 u_view;
uniform mat4 u_projection;

out vec4 frag_viewspace;
out vec3 frag_normal;
out vec3 frag_pos;
flat out float frag_light;
flat out uint frag_type;
out vec2 frag_uv;

uniform vec3 NORMALS[6] = {
  vec3( 0, 1, 0 ),
  vec3(0, -1, 0 ),
  vec3( 1, 0, 0 ),
  vec3( -1, 0, 0 ),
  vec3( 0, 0, 1 ),
  vec3( 0, 0, -1 )
};

void main() {
  frag_pos = in_pos - vec3(0.5);
  frag_viewspace = u_view * vec4(frag_pos, 1);
  frag_normal = NORMALS[in_normal];
  frag_light = float(in_light) / 32.0f;
  frag_type = in_type;
  
  gl_Position = u_projection * frag_viewspace;
}