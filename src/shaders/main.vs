#version 430 core

layout(binding = 0, std430) readonly buffer ssbo1 {
    uint data[];  
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

void main() {
  int quadIndex = int(gl_VertexID&3u);
  uint ssboIndex = quadOffset + (gl_VertexID >> 2u);

  uint quadData1 = data[ssboIndex];

  float x = float(quadData1&63u);
  float y = float((quadData1 >> 6u)&63u);
  float z = float((quadData1 >> 12u)&63u);

  float w = float((quadData1 >> 18u)&63u);
  float h = float((quadData1 >> 24u)&63u);

  // TODO
  /*
  frag_pos = vec3(x, y, z);

  int wDir = ~face & 2, lDir = (~face >> 2) & 1;
  int wMod = vertexID >> 1, lMod = vertexID & 1;
  int quadWidth = int((quadData1 >> 18u) & 63u);
  int quadLength = int((quadData1 >> 24u) & 63u);

  frag_pos[wDir] += quadWidth * wMod;
  frag_pos[lDir] += quadLength * lMod;
  */

  if (face == 0) {
    if (quadIndex == 0) {
      frag_pos = vec3(x, y, z);
    }
    else if (quadIndex == 1) {
      frag_pos = vec3(x + w, y, z);
    }
    else if (quadIndex == 2) {
      frag_pos = vec3(x + w, y, z + h);
    }
    else if (quadIndex == 3) {
      frag_pos = vec3(x, y, z + h);
    }
  }
  else if (face == 1) {
    if (quadIndex == 3) {
      frag_pos = vec3(x, y, z);
    }
    else if (quadIndex == 2) {
      frag_pos = vec3(x + w, y, z);
    }
    else if (quadIndex == 1) {
      frag_pos = vec3(x + w, y, z + h);
    }
    else if (quadIndex == 0) {
      frag_pos = vec3(x, y, z + h);
    }
  }

  else if (face == 2) {
    if (quadIndex == 3) {
      frag_pos = vec3(x, y, z);
    }
    else if (quadIndex == 2) {
      frag_pos = vec3(x, y + w, z);
    }
    else if (quadIndex == 1) {
      frag_pos = vec3(x, y + w, z + h);
    }
    else if (quadIndex == 0) {
      frag_pos = vec3(x, y, z + h);
    }
  }

  else if (face == 3) {
    if (quadIndex == 0) {
      frag_pos = vec3(x, y, z);
    }
    else if (quadIndex == 1) {
      frag_pos = vec3(x, y + w, z);
    }
    else if (quadIndex == 2) {
      frag_pos = vec3(x, y + w, z + h);
    }
    else if (quadIndex == 3) {
      frag_pos = vec3(x, y, z + h);
    }
  }

  else if (face == 4) {
    if (quadIndex == 3) {
      frag_pos = vec3(x, y, z);
    }
    else if (quadIndex == 2) {
      frag_pos = vec3(x + w, y, z);
    }
    else if (quadIndex == 1) {
      frag_pos = vec3(x + w, y + h, z);
    }
    else if (quadIndex == 0) {
      frag_pos = vec3(x, y + h, z);
    }
  }

  else if (face == 5) {
    if (quadIndex == 0) {
      frag_pos = vec3(x, y, z);
    }
    else if (quadIndex == 1) {
      frag_pos = vec3(x + w, y, z);
    }
    else if (quadIndex == 2) {
      frag_pos = vec3(x + w, y + h, z);
    }
    else if (quadIndex == 3) {
      frag_pos = vec3(x, y + h, z);
    }
  }

  frag_pos -= vec3(0.5);
  frag_viewspace = u_view * vec4(frag_pos, 1);
  frag_normal = NORMALS[face];
  frag_type = 1;
  
  gl_Position = u_projection * frag_viewspace;
}