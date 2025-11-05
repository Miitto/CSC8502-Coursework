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

layout(binding = 0) uniform sampler2D diffuse;
layout(binding = 1) uniform sampler2D depth;
layout(binding = 2) uniform samplerCube skybox;

in Vertex {
  vec2 uv;
  vec3 viewDir;
} IN;

out vec4 fragColor;

void main() {
  float depth = texture(depth, IN.uv).r;

  if (depth > 0.0) {
    vec4 diffuse = texture(diffuse, IN.uv);
    fragColor = diffuse;
  } else {
    vec4 env = texture(skybox, normalize(IN.viewDir));
    fragColor = env;
  }
}
