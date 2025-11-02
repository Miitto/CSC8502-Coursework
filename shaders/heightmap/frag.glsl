#version 460 core

#extension GL_ARB_bindless_texture : require

in Vertex {
    vec2 uv;
} IN;

layout(std140, binding = 1) uniform Textures {
    sampler2D heightmap;
    sampler2D diffuse;
} TEX;

const vec3 SRGB = vec3(2.2);

out vec4 fragColor;

void main() {
  fragColor = vec4(pow(texture(TEX.diffuse, IN.uv).rgb, 1.0 / SRGB), 1.0);
}
