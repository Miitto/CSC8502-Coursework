#version 460 core

// https://learnopengl.com/Advanced-Lighting/Bloom

out vec4 fragColor;
  
layout(binding = 1) uniform sampler2D image;
layout(binding = 0) uniform sampler2D bloom;

in Vertex {
    vec2 uv;
    vec3 viewDir;
} IN;

void main() {
  const float gamma = 2.2;

  vec3 hdrColor = texture(image, IN.uv).rgb + texture(bloom, IN.uv).rgb;

  vec3 reinhard = hdrColor / (hdrColor + vec3(1.0));

  // Convert back to sRGB on output
  fragColor = vec4(pow(reinhard, vec3(1.0 / gamma)), 1.0);
}
