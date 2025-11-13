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

layout(location = 0) in vec3 position;

out Vertex {
  vec3 lightPos;
  float lightRadius;
  vec4 lightColor;
} OUT;

void main() {
    vec3 scale = vec3(lightRadius);
    vec3 worldPos = (position * scale) + lightPos;

    gl_Position = CAM.viewProj * vec4(worldPos, 1.0);

    OUT.lightPos = lightPos;
    OUT.lightRadius = lightRadius;
    OUT.lightColor = lightColor;
}

