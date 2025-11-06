#version 460 core

layout(binding = 0) uniform sampler2D diffuse;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D material;
layout(binding = 3) uniform sampler2D depth;
layout(binding = 4) uniform samplerCube skybox;
layout(binding = 5) uniform sampler2D diffuseLight;
layout(binding = 6) uniform sampler2D specularLight;



in Vertex {
 vec2 uv;
 vec3 viewDir;
} IN;

out vec4 fragColor;

void main() {
  vec4 diffuse = texture(diffuse, IN.uv);
  vec3 normal = texture(normal, IN.uv).xyz * 2.0 - 1.0;
  vec4 material = texture(material, IN.uv);
  float reflectivity = material.a;

  if (reflectivity == 0.0) {
    fragColor = diffuse;
    return;
  }

  vec3 reflectDir = reflect(normalize(IN.viewDir), normalize(normal));
  vec4 env = texture(skybox, reflectDir);

  fragColor = mix(diffuse, env, reflectivity);
}
