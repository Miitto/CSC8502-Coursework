#version 460 core

layout(binding = 0) uniform sampler2D depthMap;

layout(location = 0) uniform float near;
layout(location = 1) uniform float far;

in Vertex {
  vec2 uv;
  vec3 viewDir;
} IN;

out vec4 fragColor;

void main() {
    float depth = texture(depthMap, IN.uv).r;

    float eye_z = near * far / ((depth * (far - near)) - far);
    float vis = ( eye_z + near ) / ( near - far );

    fragColor = vec4(vec3(vis), 1.0);
}
