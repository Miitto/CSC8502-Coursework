#version 460 core

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
    vec2 resolution;
    vec2 uvRange;
} CAM;


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
  vec2 uv = IN.uv;
  uv.x = mix(CAM.uvRange.x, CAM.uvRange.y, uv.x);

  vec4 diffuse = texture(diffuse, uv);
  vec3 normal = texture(normal, uv).xyz * 2.0 - 1.0;
  vec4 material = texture(material, uv);
  float reflectivity = material.a;

  if (reflectivity == 0.0) {
    fragColor = diffuse;
    return;
  }

  vec3 reflectDir = reflect(normalize(IN.viewDir), normalize(normal));
  vec4 env = texture(skybox, reflectDir);

  fragColor = mix(diffuse, env, reflectivity);
}
