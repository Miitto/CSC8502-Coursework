#version 460 core

const vec4 POS[] = vec4[](
    vec4(-1.0, 0.0, -1.0, 1.0),
    vec4(-1.0, 0.0,  1.0, 1.0),
    vec4( 1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0,  1.0, 1.0)
);

layout(location = 0) uniform float size;
layout(location = 1) uniform float yLevel;
layout(location = 2) uniform float tileCount;

void main() {
  vec3 pos = POS[gl_VertexID].xyz * size;
  pos.y = yLevel;
  gl_Position = vec4(pos, 1.0);
}
