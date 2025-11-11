#version 460 core

const vec3 POS[] = vec3[](
    vec3(-1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
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
    vec2 uvRange;
} CAM;

layout(location = 0) uniform vec3 lightPos;
layout(location = 1) uniform float lightRadius;
layout(location = 2) uniform vec4 lightColor;

out Vertex {
  vec3 lightPos;
  float lightRadius;
  vec4 lightColor;
} OUT;

void main() {
    vec3 scale = vec3(lightRadius);
    vec3 worldPos = (POS[INDICES[gl_VertexID]] * scale) + lightPos;

    gl_Position = CAM.viewProj * vec4(worldPos, 1.0);

    OUT.lightPos = lightPos;
    OUT.lightRadius = lightRadius;
    OUT.lightColor = lightColor;
}

