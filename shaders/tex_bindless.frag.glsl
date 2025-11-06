#version 460

#extension GL_ARB_bindless_texture : require

in Vertex {
  vec2 uv;
  vec3 normal;
  vec3 tangent;
  vec3 binormal;
  flat int drawID;
} IN;

struct TextureSet {
  uvec2 diffuse;
  uvec2 bump;
  uvec2 material;
};

layout(binding = 2, std430) readonly buffer Textures {
    TextureSet textures[];
} TEXTURES;

bool isTextureValid(uvec2 tex) {
  return (tex.x != 0u || tex.y != 0u);
}

layout(location = 0) out vec4 diffuseOut;
layout(location = 1) out vec4 normalOut;
layout(location = 2) out vec4 materialOut;

void main() {
  TextureSet tex = TEXTURES.textures[IN.drawID];


  // Diffuse MUST be valid (i hope)
  vec4 diffuse = texture(sampler2D(tex.diffuse), IN.uv);

  vec3 normal = IN.normal;
  if (isTextureValid(tex.bump)) {
    vec3 bump = texture(sampler2D(tex.bump), IN.uv).xyz * 2.0 - 1.0;

    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

    normal = normalize(TBN * normalize(bump));
  }

  materialOut = vec4(0.0, 0.3, 0.3, 0.0);
  if (isTextureValid(tex.material)) {
    materialOut = texture(sampler2D(tex.material), IN.uv);
  }

  diffuseOut = diffuse;
  normalOut = vec4(normal * 0.5 + 0.5, 1.0);
}
