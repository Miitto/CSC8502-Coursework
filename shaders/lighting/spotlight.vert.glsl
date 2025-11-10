#version 460 core

const vec3 POS[] = vec3[](
    vec3(-1.0, -1.0, .0),
    vec3( 1.0, -1.0, .0),
    vec3(-1.0,  1.0, .0),
    vec3( 1.0,  1.0, .0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3(-1.0,  1.0,  1.0),
    vec3( 1.0,  1.0,  1.0)
);

const uint INDICES[] = uint[](
    0, 1, 2,  1, 3, 2,
    4, 6, 5,  5, 6, 7,
    0, 2, 4,  4, 2, 6,
    1, 5, 3,  5, 7, 3,
    2, 3, 6,  3, 7, 6,
    0, 4, 1,  1, 4, 5
);

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
    vec2 resolution;
} CAM;

layout(location = 0) uniform mat4 modelMatrix;
layout(location = 2) uniform vec4 lightColor;

out Vertex {
  vec3 lightPos;
  float lightRadius;
  vec4 lightColor;
} OUT;

void main() {
    vec4 worldPos = modelMatrix * vec4(POS[INDICES[gl_VertexID]], 1.0);

    gl_Position = CAM.viewProj * worldPos;
    
    OUT.lightColor = lightColor;
}

