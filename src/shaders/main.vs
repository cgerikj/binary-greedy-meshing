#version 430 core

layout(binding = 0, std430) readonly buffer ssbo1 {
    uint data[];  
};

uniform mat4 u_view;
uniform mat4 u_projection;

out vec4 frag_viewspace;
out vec3 frag_pos;
out vec3 frag_normal;
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
  uint quadData = data[gl_VertexID];

  float x = float(quadData&63u);
  float y = float((quadData >> 6)&63u);
  float z = float((quadData >> 12)&63u);
  
  // float w = float((quadData >> 18)&63u);
  // float h = float((quadData >> 24)&63u);

  frag_pos = vec3(x, y, z) - vec3(0.5);
  frag_viewspace = u_view * vec4(frag_pos, 1);
  frag_normal = NORMALS[0];
  frag_type = 1;
  
  gl_Position = u_projection * frag_viewspace;
} 