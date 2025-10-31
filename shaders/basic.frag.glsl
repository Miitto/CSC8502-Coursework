#version 460 core

in Vertex {
  vec4 color;
} IN;

out vec4 fragColor;

void main() {
    fragColor = IN.color;
}