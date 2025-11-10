#version 460 core

layout(std140, binding = 5) uniform LightUniforms {
  mat4 shadowMatrix[6];
  vec3 lightPos;
  float radius;
} U;


layout(location = 0) in vec3 position;
layout(location = 4) in mat4 modelMatrix;

out Vertex {
    vec4 fragPos;
    flat vec3 lightPos;
    flat float radius;
} OUT;

void main() {
  OUT.fragPos = modelMatrix * vec4(position, 1.0);
  OUT.lightPos = U.lightPos;
  OUT.radius = U.radius;

  gl_Position = modelMatrix * vec4(position, 1.0);
}
