#version 460 core

layout(location = 0) uniform vec3 scale;
layout(location = 1) uniform uint chunksPerAxis; 

const vec3 POS[4] = vec3[](
    vec3(0.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 1.0)
);

const vec2 UVS[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

out Vertex {
    vec2 uv;
} OUT;

void main() {
  uint chunkIndex = gl_VertexID / 4u;

  float chunkX = chunkIndex % chunksPerAxis;
  float chunkZ = chunkIndex / chunksPerAxis;

  vec2 baseUv = UVS[gl_VertexID % 4];

  vec2 uvOffset = vec2(chunkX, chunkZ) / float(chunksPerAxis);

  vec2 uv = baseUv / float(chunksPerAxis) + uvOffset;

  chunkX -= float(chunksPerAxis) / 2.;
  chunkZ -= float(chunksPerAxis) / 2.;

  vec3 pos = POS[gl_VertexID % 4];
  pos.x += chunkX;
  pos.z += chunkZ;

  gl_Position = vec4(pos * (scale / chunksPerAxis), 1.0);
  OUT.uv = uv;
}
