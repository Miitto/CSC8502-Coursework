#version 460 core

layout(binding = 0) uniform sampler2D diffuseTex;
layout(binding = 1) uniform sampler2D diffuseLight;
layout(binding = 2) uniform sampler2D specularLight;

layout(location = 0) uniform vec3 ambient = vec3(0.1);

in Vertex {
  vec2 uv;
  vec3 viewDir;
} IN;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 brightColor;

layout(location = 1) uniform bool adjust = true;

void main() {
  // Convert diffuse to RGB for lighting
  vec3 diffuse = pow(texture(diffuseTex, IN.uv).rgb, vec3(2.2));
  vec3 light = texture(diffuseLight, IN.uv).rgb;
  vec3 specular = texture(specularLight, IN.uv).rgb;

  fragColor.rgb = diffuse * ambient; // Ambient
  fragColor.rgb += diffuse * light; // Lambert
  fragColor.rgb += specular; // Specular
  fragColor.a = 1.0;


  float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));

  if (adjust) {

    // Tone map
    fragColor.rgb = fragColor.rgb / (fragColor.rgb + vec3(1.0));

    // Convert back to sRGB on output
    fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / 2.2));
  }

  if(brightness > 1.0)
    brightColor = vec4(fragColor.rgb, 1.0);
  else
    brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
