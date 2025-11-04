#version 460 core

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
    vec2 resolution;
} CAM;

layout(std430, binding = 1) buffer ModelMats {
    mat4 model[];
} MODEL;

layout(location = 0) uniform int modelIndex;
layout(location = 1) uniform vec4 nodeColor;

layout(location = 0) in vec3 position;
layout(location = 2) in vec4 color;

out Vertex {
  vec4 color;
} OUT;

void main() {
    vec4 worldPos = MODEL.model[modelIndex] * vec4(position, 1.0);
    gl_Position = CAM.viewProj * worldPos;
    OUT.color = nodeColor;
}
