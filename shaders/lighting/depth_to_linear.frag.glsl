#version 460 core

in Vertex {
    vec4 fragPos;
    flat vec3 lightPos;
    flat float radius;
} IN;

void main() {
    float lightDistance = length(IN.fragPos.xyz - IN.lightPos);
    
    lightDistance = 1.0 - (lightDistance / IN.radius);
    
    gl_FragDepth = lightDistance;
}  