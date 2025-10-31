#version 460 core

in Vertex {
  vec2 uv;
} IN;

layout(binding = 0) uniform sampler2D diffuse;

out vec4 fragColor;

void main() {
    fragColor = texture(diffuse, IN.uv);
}