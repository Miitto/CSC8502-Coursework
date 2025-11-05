#version 460

in Vertex {
  vec2 uv;
  vec3 viewDir;
} IN;

layout(binding = 0) uniform sampler2D diffuse;

out vec4 fragColor;

void main() {
    fragColor = texture(diffuse, IN.uv);
}