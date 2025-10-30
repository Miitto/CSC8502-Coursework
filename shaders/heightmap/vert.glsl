#version 460 core

layout(location = 0) uniform vec3 scale;

const vec3 POS[4] = vec3[](
    vec3(-1.0, 0.0, -1.0),
    vec3( 1.0, 0.0, -1.0),
    vec3(-1.0, 0.0,  1.0),
    vec3( 1.0, 0.0,  1.0)
);

const vec2 UVS[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

out Vertex {
    vec2 uv;
} OUT;

void main() {
  gl_Position = vec4(POS[gl_VertexID] * scale, 1.0);
  OUT.uv = UVS[gl_VertexID];
}
