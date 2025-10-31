#version 460 core

const vec4 POS[3] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4( 3.0, -1.0, 0.0, 1.0),
    vec4(-1.0,  3.0, 0.0, 1.0)
);

out Vertex {
 vec2 uv;
} OUT;

void main() {
    gl_Position = POS[gl_VertexID];

    OUT.uv = (gl_Position.xy + vec2(1.0)) * 0.5;
}

