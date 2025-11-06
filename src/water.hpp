#pragma once

#include <engine/globals.hpp>
#include <engine/scene_node.hpp>
#include <gl/gl.hpp>

class Water : public engine::scene::Node {
public:
  Water(float size, float yLevel, const gl::CubeMap& envMap)
      : engine::scene::Node({engine::scene::Node::RenderType::LIT, true}),
        size(size), yLevel(yLevel), envMap(envMap) {
    auto waterProgOpt = gl::Program::fromFiles(
        {{SHADERDIR "water.vert.glsl", gl::Shader::Type::VERTEX},
         {SHADERDIR "water.frag.glsl", gl::Shader::Type::FRAGMENT}});
    if (!waterProgOpt) {
      Logger::error("Failed to create water program: {}", waterProgOpt.error());
      throw std::runtime_error("Failed to create water program");
    }
    waterProgram = std::move(*waterProgOpt);

    auto diffuseImgOpt = engine::Image::fromFile(TEXTUREDIR "water.tga", true);
    if (!diffuseImgOpt) {
      Logger::error("Failed to load water diffuse texture: {}",
                    diffuseImgOpt.error());
      throw std::runtime_error("Failed to load water diffuse texture");
    }
    diffuseMap = diffuseImgOpt->toTexture(-1);
    diffuseMap.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    diffuseMap.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    diffuseMap.setParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
    diffuseMap.setParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
    diffuseMap.setParameter(GL_TEXTURE_MAX_ANISOTROPY, 16);

    auto bumpImgOpt = engine::Image::fromFile(TEXTUREDIR "waterbump.png", true);
    if (!bumpImgOpt) {
      Logger::error("Failed to load water bump texture: {}",
                    bumpImgOpt.error());
      throw std::runtime_error("Failed to load water bump texture");
    }
    bumpMap = bumpImgOpt->toTexture();
    bumpMap.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    bumpMap.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SetBoundingRadius(size);
    glm::mat4 transform = glm::mat4(1.0f);
    transform[3][1] = yLevel;
    SetTransform(transform);
  }

  void update(const engine::FrameInfo& frame) override { (void)frame; }
  void render(const engine::FrameInfo& info, const engine::Camera& camera,
              const engine::Frustum& frustum) override {
    (void)info;
    (void)frustum;
    (void)camera;
    auto bg = engine::globals::DUMMY_VAO.bindGuard();
    waterProgram.bind();
    diffuseMap.bind(0);
    bumpMap.bind(1);

    glUniform1f(0, size);
    glUniform1f(1, yLevel);
    glUniform1f(2, 10.f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

private:
  float size;
  float yLevel;

  gl::Program waterProgram;
  gl::Texture diffuseMap;
  gl::Texture bumpMap;
  const gl::CubeMap& envMap;
};