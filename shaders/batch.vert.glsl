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

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec4 tangent;
layout(location = 7) in mat4 modelMatrix;

out Vertex {
  vec2 uv;
  vec3 normal;
  vec3 tangent;
  vec3 binormal;
  flat int drawID;
} OUT;

void main() {
  vec4 local = vec4(position, 1.0);

  mat4 mvp = CAM.viewProj * modelMatrix;
  gl_Position = mvp * local;

  OUT.uv = uv;
  OUT.normal = normalize(mat3(modelMatrix) * normal);
  OUT.tangent = normalize(mat3(modelMatrix) * tangent.xyz);
  OUT.binormal = normalize(cross(OUT.tangent, OUT.normal) * tangent.w);
  OUT.drawID = gl_DrawID;
}
