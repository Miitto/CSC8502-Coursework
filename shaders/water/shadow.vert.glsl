#version 460 core

layout(std140, binding = 5) uniform LightUniforms {
  mat4 shadowMatrix;
  vec3 lightPos;
  float radius;
} U;

const vec4 POS[] = vec4[](
    vec4(-1.0, 0.0, -1.0, 1.0),
    vec4(-1.0, 0.0,  1.0, 1.0),
    vec4( 1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0,  1.0, 1.0)
);

layout(location = 0) uniform float size;
layout(location = 1) uniform float yLevel;
layout(location = 2) uniform float tileCount;

out Vertex {
    vec4 fragPos;
    flat vec3 lightPos;
    flat float radius;
} OUT;


void main() {
  vec3 pos = POS[gl_VertexID].xyz * size;
  pos.y = yLevel;

  OUT.fragPos = vec4(pos, 1.0);
  OUT.lightPos = U.lightPos;
  OUT.radius = U.radius;

  gl_Position = U.shadowMatrix * vec4(pos, 1.0);
}
