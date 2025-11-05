#version 460 core

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D diffuseLight;
layout(binding = 2) uniform sampler2D specularLight;

layout(location = 0) uniform vec3 ambient = vec3(0.1);

in Vertex {
  vec2 uv;
  vec3 viewDir;
} IN;

out vec4 fragColor;

void main() {
  vec3 diffuse = texture(diffuseTex, IN.uv).rgb;
  vec3 light = texture(diffuseLight, IN.uv).rgb;
  vec3 specular = texture(specularLight, IN.uv).rgb;

  fragColor.rgb = diffuse * ambient; // Ambient
  fragColor.rgb += diffuse * light; // Lambert
  fragColor.rgb += specular; // Specular
  fragColor.a = 1.0;
}
