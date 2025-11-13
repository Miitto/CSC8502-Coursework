#version 460 core

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
    vec2 resolution;
    vec2 uvRange;
} CAM;

layout(location = 0) uniform vec3 lightPos;
layout(location = 1) uniform float lightRadius;
layout(location = 2) uniform vec4 lightColor;
layout(location = 3) uniform vec3 lightDirection;
layout(location = 4) uniform mat4 modelMatrix;

layout(location = 0) in vec3 position;

out Vertex {
  vec3 lightPos;
  float lightRadius;
  vec4 lightColor;
  vec3 lightDirection;
} OUT;

void main() {
    vec3 worldPos = (modelMatrix * vec4(position, 1.0)).xyz;

    gl_Position = CAM.viewProj * vec4(worldPos, 1.0);

    OUT.lightPos = lightPos;
    OUT.lightRadius = lightRadius;
    OUT.lightColor = lightColor;
    OUT.lightDirection = lightDirection;
}

