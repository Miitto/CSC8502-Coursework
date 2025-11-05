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

layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D waterBump;

in Vertex {
  vec2 uv;
} IN;

layout(location = 0) out vec4 diffuseOut;
layout(location = 1) out vec4 normalOut;
layout(location = 2) out vec4 materialOut;


void main() {
  vec3 diffuse = texture(diffuseMap, IN.uv).rgb;
  vec3 normal = texture(waterBump, IN.uv).rgb * 2.0 - 1.0;

  diffuseOut = vec4(diffuse, 1.0);
  normalOut = vec4(normalize(normal), 1.0);
  materialOut = vec4(0.0, 0.0, 1.0, 1.0);
}