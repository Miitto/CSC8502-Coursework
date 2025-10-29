#version 460 core

in Vertex {
  vec4 color;
} IN;

out vec4 fragColor;

const float SRGB = 2.2;

void main() {
    fragColor = vec4(pow(IN.color.rgb, vec3(1.0 / SRGB)), IN.color.a);
}