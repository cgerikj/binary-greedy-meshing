#version 430 core

struct QuadData {
  uint quadData1;
  uint quadData2;
};

layout(binding = 0, std430) readonly buffer ssbo1 {
  QuadData data[];  
};

uniform mat4 u_view;
uniform mat4 u_projection;

uniform int face;
uniform int quadOffset;

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

const int flipLookup[6] = int[6](1, -1, -1, 1, -1, 1);

void main() {
  int quadIndex = int(gl_VertexID&3u);
  uint ssboIndex = quadOffset + (gl_VertexID >> 2u);

  uint quadData1 = data[ssboIndex].quadData1;
  uint quadData2 = data[ssboIndex].quadData2;

  float x = float(quadData1&63u);
  float y = float((quadData1 >> 6u)&63u);
  float z = float((quadData1 >> 12u)&63u);

  float w = float((quadData1 >> 18u)&63u);
  float h = float((quadData1 >> 24u)&63u);

  int wDir = (face & 2) >> 1, hDir = 2 - (face >> 2);
  int wMod = quadIndex >> 1, hMod = quadIndex & 1;

  frag_pos = vec3(x, y, z);
  frag_pos[wDir] += w * wMod * flipLookup[face];
  frag_pos[hDir] += h * hMod;

  uint type = quadData2&255u;

  frag_pos -= vec3(0.5);
  frag_viewspace = u_view * vec4(frag_pos, 1);
  frag_normal = NORMALS[face];
  frag_type = type;
  
  gl_Position = u_projection * frag_viewspace;
}