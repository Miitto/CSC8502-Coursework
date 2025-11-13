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
    float time;
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
  vec2 diffuseOffset = vec2(CAM.time * 0.025, CAM.time * 0.0125);
  vec3 diffuse = texture(diffuseMap, IN.uv + diffuseOffset).rgb;

  vec2 bumpOffset = vec2(CAM.time * 0.05, CAM.time * 0.0125);

  vec3 bump = texture(waterBump, IN.uv + bumpOffset).rgb * 2.0 - 1.0;

  vec3 tangent = vec3(1.0, 0.0, 0.0);
  vec3 binormal = vec3(0.0, 0.0, 1.0);
  vec3 normal = vec3(0.0, 1.0, 0.0);

  mat3 TBN = mat3(normalize(tangent), normalize(binormal), normalize(normal));

  vec3 bumpedNormal = normalize(TBN * normalize(bump));

  diffuseOut = vec4(diffuse, 1.0);
  normalOut = vec4(normalize(bumpedNormal), 1.0);
  materialOut = vec4(0.8, 0.0, 0.2, 0.0);
}