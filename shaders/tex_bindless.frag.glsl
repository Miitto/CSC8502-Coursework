#version 460

#extension GL_ARB_bindless_texture : require

in Vertex {
  vec2 uv;
  flat int drawID;
} IN;

layout(binding = 2, std430) readonly buffer DiffuseTextures {
    sampler2D textures[];
} diffuseTextures;

out vec4 fragColor;

void main() {
  sampler2D tex = diffuseTextures.textures[IN.drawID];
  fragColor = texture(tex, IN.uv);
}
