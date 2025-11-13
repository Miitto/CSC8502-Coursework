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

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D normalTex;
layout(binding = 2) uniform sampler2D materialTex;
layout(binding = 3) uniform sampler2D depthTex;
layout(binding = 4) uniform samplerCube shadowMap;

const float PI = 3.14159265359;

in Vertex {
  vec3 lightPos;
  float lightRadius;
  vec4 lightColor;
  vec3 lightDirection;
} IN;

layout(location = 0) out vec4 diffuseOut;
layout(location = 1) out vec4 specularOut;

float calculateOcclusion(vec3 fragPos) {
  vec3 worldToLight = fragPos - IN.lightPos;
  float depthCenter = texture(shadowMap, worldToLight).r;

  float offset = 1.0f / 4096.0f; // assuming 2048x2048 shadow map resolution

  float depthRight = texture(shadowMap, worldToLight + vec3(offset, 0.0, 0.0)).r;
  float depthLeft = texture(shadowMap, worldToLight + vec3(-offset, 0.0, 0.0)).r;
  float depthUp = texture(shadowMap, worldToLight + vec3(0.0, offset, 0.0)).r;
  float depthDown =  texture(shadowMap, worldToLight + vec3(0.0, -offset, 0.0)).r;
  float depth = 1.0 - (depthCenter + depthRight + depthLeft + depthUp + depthDown) / 5.0;

  depth *= IN.lightRadius;

  float currentDepth = length(worldToLight);

  float bias = 0.05;
  float shadow = currentDepth - bias > depth ? 0.0 : 1.0;
  return shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main() {
  vec2 uv;
  uv.y = gl_FragCoord.y / CAM.resolution.y;

  float uvRange = CAM.uvRange.y - CAM.uvRange.x;

  float windowX = CAM.resolution.x / uvRange;

  float fragPercentage = gl_FragCoord.x / windowX;

  uv.x = fragPercentage;

  // UV coord of this fragment relative to the viewport, not the window
  float viewportX = (fragPercentage - CAM.uvRange.x) / uvRange;
  
  float depth = texture(depthTex, uv).r;
  vec3 ndc = vec3(vec2(viewportX, uv.y), depth) * 2.0 - 1.0;
  vec4 invClip = CAM.invViewProj * vec4(ndc, 1.0);
  vec3 world = invClip.xyz / invClip.w;

  float dist = length(IN.lightPos - world);

  float distAtten = 1.0 - clamp(dist / IN.lightRadius, 0.0, 1.0);

  vec3 incident = normalize(IN.lightPos - world);

  float theta = dot(incident, normalize(IN.lightDirection));
  float epsilon = 0.75;
  float spotAtten = clamp((theta - epsilon) / (1.0 - epsilon), 0.0, 1.0);
  
  float atten = distAtten * spotAtten;

  if (atten == 0.0) {
      discard;
  }

  vec4 aSample = texture(diffuseTex, uv);
  vec3 albedo = pow(aSample.rgb, vec3(2.2)) * aSample.a;
  vec4 material = texture(materialTex, uv);
  float metallic = material.g;
  float roughness = material.b;
  float ao = material.a;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo.rgb, metallic);

  vec3 camPos = CAM.invView[3].xyz;

  vec3 normal = normalize(texture(normalTex, uv).xyz);

  vec3 viewDir = normalize(camPos - world);
  vec3 halfDir = normalize(incident + viewDir);

  vec3 radiance = IN.lightColor.rgb * IN.lightColor.a;
  radiance *= atten;

  float NDF = DistributionGGX(normal, halfDir, roughness);
  float G = GeometrySmith(normal, viewDir, incident, roughness);
  vec3 F = fresnelSchlick(max(dot(halfDir, viewDir), 0.0), F0);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - F;
  kD *= 1.0 - metallic;
  
  vec3 numerator = NDF * G * F;
  float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, incident), 0.0) + 0.001;
  vec3 specular = numerator / denominator;

  float NdotL = clamp(dot(normal, incident), 0.0, 1.0);
  
  float shadowOcclusion = calculateOcclusion(world);
  radiance *= shadowOcclusion;

  specularOut = vec4(specular * radiance * NdotL, 1.0);

  vec3 adjusted = kD * (albedo.rgb / PI + specular) * NdotL * radiance;

  diffuseOut = vec4(adjusted, 1.0);
}