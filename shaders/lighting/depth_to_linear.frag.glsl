#version 460 core

layout(std140, binding = 5) uniform LightUniforms {
  mat4 shadowMatrix[6];
  vec3 lightPos;
  float radius;
} U;

in Vertex {
    vec4 fragPos;
} IN;

void main()
{
    float lightDistance = length(IN.fragPos.xyz - U.lightPos);
    
    lightDistance = 1.0 - (lightDistance / U.radius);
    
    gl_FragDepth = lightDistance;
}  