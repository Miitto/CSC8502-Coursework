#include "heightmap.hpp"
#include "logger/logger.hpp"
#include <engine/camera.hpp>
#include <engine/gui.hpp>
#include <engine\image.hpp>

std::expected<Heightmap, std::string>
Heightmap::fromFile(std::string_view heightFile, std::string_view diffuseFile,
                    std::string_view normalFile) {
  Logger::debug("Loading heightmap from heightFile: {}", heightFile);
  auto heightImgRes = engine::Image::fromFile(heightFile, 1);

  if (!heightImgRes.has_value()) {
    return std::unexpected(heightImgRes.error());
  }
  auto& heightImg = heightImgRes.value();

  auto heightTex = heightImg.toTexture();
  heightTex.generateMipmap();
  heightTex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  heightTex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto diffuseImgRes = engine::Image::fromFile(diffuseFile);

  if (!diffuseImgRes.has_value()) {
    return std::unexpected(diffuseImgRes.error());
  }
  auto& diffuseImg = diffuseImgRes.value();

  auto diffuseTex = diffuseImg.toTexture();
  diffuseTex.generateMipmap();
  diffuseTex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  diffuseTex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto normalImgRes = engine::Image::fromFile(normalFile);

  if (!normalImgRes.has_value()) {
    return std::unexpected(normalImgRes.error());
  }
  auto& normalImg = normalImgRes.value();

  auto normalTex = normalImg.toTexture();
  normalTex.generateMipmap();
  normalTex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  normalTex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto progOpt = gl::Program::fromFiles({
      {SHADERDIR "heightmap/vert.glsl", gl::Shader::Type::VERTEX},
      {SHADERDIR "heightmap/tess_con.glsl", gl::Shader::Type::TESS_CONTROL},
      {SHADERDIR "heightmap/tess_eval.glsl", gl::Shader::Type::TESS_EVAL},
      {SHADERDIR "heightmap/frag.glsl", gl::Shader::Type::FRAGMENT},
  });
  if (!progOpt) {
    return std::unexpected(progOpt.error());
  }
  auto& prog = progOpt.value();

  return Heightmap(std::move(heightTex), std::move(diffuseTex),
                   std::move(normalTex), std::move(prog));
}

void Heightmap::render(const engine::FrameInfo& info,
                       const engine::Camera& camera,
                       const engine::Frustum& frustum) {
  program.bind();

  camera.bindMatrixBuffer(0);

  auto bg = dummyVao.bindGuard();

  heightTex.bind(0);
  diffuseTex.bind(1);
  normalTex.bind(2);

  constexpr int chunksPerAxis = 7;

  glPatchParameteri(GL_PATCH_VERTICES, 4);
  glDrawArrays(GL_PATCHES, 0, 4 * chunksPerAxis * chunksPerAxis);

  engine::scene::Node::render(info, camera, frustum);
}
