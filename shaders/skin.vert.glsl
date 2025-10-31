#version 460 core

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
} CAM;

layout(location = 0) uniform mat4 model;
layout(location = 4) uniform mat4 joints[128];

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;
layout(location = 5) in vec4 joinWeights;
layout(location = 6) in ivec4 jointIndices;

out Vertex {
  vec2 uv;
  flat int drawID;
} OUT;

void main() {
  vec4 local = vec4(position, 1.0);
  vec4 skelPos = vec4(0.0);

  for (int i = 0; i < 4; ++i) {
    int jointIndex = jointIndices[i];
    float weight = joinWeights[i];

    skelPos += joints[jointIndex] * local * weight;
  }

  mat4 mvp = CAM.viewProj * model;
  gl_Position = mvp * vec4(skelPos.xyz, 1.0);

  OUT.uv = uv;
  OUT.drawID = gl_DrawID;
}
