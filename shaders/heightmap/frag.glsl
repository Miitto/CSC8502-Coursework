#version 460 core

in Vertex {
  vec2 uv;
} IN;

layout(binding = 1) uniform sampler2D diffuse;
layout(binding = 2) uniform sampler2D normalMap;

const vec3 SRGB = vec3(2.2);

layout(location = 0) out vec4 diffuseOut;
layout(location = 1) out vec4 normalOut;
layout(location = 2) out vec4 materialOut;

void main() {
  diffuseOut = vec4(pow(texture(diffuse, IN.uv).rgb, 1.0 / SRGB), 1.0);
  normalOut = vec4(normalize(texture(normalMap, IN.uv).rgb), 1.0);
  materialOut = vec4(0.0, 0.0, 0.9, 0.0);
}
