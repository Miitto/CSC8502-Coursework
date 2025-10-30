#version 460 core

in Vertex {
    vec2 uv;
    float height;
} IN;

layout(location = 0) uniform vec3 scale;

out vec4 fragColor;

void main() {
  float h = IN.height / scale.y;
  fragColor = vec4(h, h, h, 1.0);
}
