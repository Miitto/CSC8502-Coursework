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
} CAM;


const vec4 POS[3] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4( 3.0, -1.0, 0.0, 1.0),
    vec4(-1.0,  3.0, 0.0, 1.0)
);

out Vertex {
 vec2 uv;
 vec3 viewDir;
} OUT;

void main() {
    gl_Position = POS[gl_VertexID];

    OUT.viewDir = mat3(CAM.invView) * (CAM.invProj * gl_Position).xyz;
    OUT.uv = (gl_Position.xy + vec2(1.0)) * 0.5;
}

