#version 460 core

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
    vec2 resolution;
} CAM;

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D materialTex;
layout(binding = 3) uniform sampler2D depthTex;

in Vertex {
    vec3 lightPos;
    float lightRadius;
    vec4 lightColor;
} IN;

layout(location = 0) out vec4 diffuseOut;
layout(location = 1) out vec4 specularOut;

void main() {
  vec2 uv = gl_FragCoord.xy / CAM.resolution;

  float depth = texture(depthTex, uv).r;
  vec3 ndc = vec3(uv, depth) * 2.0 - 1.0;
  vec4 invClip = CAM.invViewProj * vec4(ndc, 1.0);
  vec3 world = invClip.xyz / invClip.w;

  float dist = length(IN.lightPos - world);

  float atten = 1.0 - clamp(dist / IN.lightRadius, 0.0, 1.0);

  if (atten == 0.0) {
      discard;
  }


  vec4 material = texture(materialTex, uv);
  float emissivity = material.r;
  float roughness = material.g;
  float specular = material.b;

  vec3 camPos = CAM.invView[3].xyz;

  vec3 sampledNormal = texture(normalTex, uv).xyz;
  vec3 normal = normalize(sampledNormal * 2.0 - 1.0);

  vec3 incident = normalize(IN.lightPos - world);
  vec3 viewDir = normalize(camPos - world);
  vec3 halfDir = normalize(incident + viewDir);

  float lambert = clamp(dot(incident, normal), 0.0, 1.0);
  float rFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
  float specFactor = pow(rFactor, mix(1.0, 256.0,roughness)) * specular;
  vec3 attenuated = IN.lightColor.rgb * atten;

  diffuseOut = vec4(max(attenuated * lambert, vec3(emissivity)), 1.0);
  specularOut = vec4(attenuated * specFactor, 1.0);
}