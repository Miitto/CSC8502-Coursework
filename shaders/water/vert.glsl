#version 460 core

const vec4 POS[] = vec4[](
    vec4(-1.0, 0.0, -1.0, 1.0),
    vec4(-1.0, 0.0,  1.0, 1.0),
    vec4( 1.0, 0.0, -1.0, 1.0),
    vec4( 1.0, 0.0,  1.0, 1.0)
);

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


layout(location = 0) uniform float size;
layout(location = 1) uniform float yLevel;
layout(location = 2) uniform float tileCount;

out Vertex {
    vec2 uv;
} OUT;

void main() {
  vec3 pos = POS[gl_VertexID].xyz * size;
  pos.y = yLevel;
  gl_Position = CAM.viewProj * vec4(pos, 1.0);

  OUT.uv = (POS[gl_VertexID].xz + vec2(1.0)) * 0.5 * tileCount;
}
