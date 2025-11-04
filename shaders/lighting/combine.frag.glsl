#version 460 core

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D diffuseLight;
layout(binding = 2) uniform sampler2D specularLight;

in Vertex {
  vec2 uv;
} IN;

out vec4 fragColor;

void main() {
  vec3 diffuse = texture(diffuseTex, IN.uv).rgb;
  vec3 light = texture(diffuseLight, IN.uv).rgb;
  vec3 specular = texture(specularLight, IN.uv).rgb;

  fragColor.rgb = diffuse * 0.1; // Ambient
  fragColor.rgb += diffuse * light; // Lambert
  fragColor.rgb += specular; // Specular
  fragColor.a = 1.0;
}
