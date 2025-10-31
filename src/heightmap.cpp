#include "heightmap.hpp"
#include "logger/logger.hpp"
#include <engine/camera.hpp>
#include <engine/gui.hpp>
#include <engine\image.hpp>

std::expected<Heightmap, std::string>
Heightmap::fromFile(std::string_view file) {
  Logger::debug("Loading heightmap from file: {}", file);
  auto imgOpt = engine::Image::fromFile(file);

  if (!imgOpt.has_value()) {
    return std::unexpected(imgOpt.error());
  }
  auto& img = imgOpt.value();

  auto tex = img.toTexture();
  tex.generateMipmap();
  tex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  tex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

  gl::StorageBuffer lodBuffer(
      sizeof(LodParams), nullptr,
      gl::Buffer::Usage::WRITE | gl::Buffer::Usage::PERSISTENT |
          gl::Buffer::Usage::COHERENT | gl::Buffer::Usage::DYNAMIC);
  lodBuffer.label("Heightmap LOD Params Buffer");
  lodBuffer.map(gl::Buffer::Mapping::WRITE | gl::Buffer::Mapping::PERSISTENT |
                gl::Buffer::Mapping::COHERENT);

  return Heightmap(std::move(tex), std::move(prog), std::move(lodBuffer));
}

void Heightmap::render(const engine::FrameInfo& info,
                       const engine::Camera& camera) {
  (void)info;
  {
    engine::gui::GuiWindow frame("Heightmap");
    ImGui::InputFloat3("Scale", &scale.x);
    bool updated = ImGui::InputFloat4("LOD Params", &lodParams.minDistance);
    if (updated) {
      Logger::debug("Updating heightmap LOD params");
      writeLodParams();
    }
  }

  program.bind();
  texture.bind(0);

  camera.bindMatrixBuffer(0);
  lodBuffer.bindBase(gl::StorageBuffer::Target::UNIFORM, 1);
  SetBoundingRadius(std::max(scale.x, scale.z) * 0.5f);

  glUniform3f(0, scale.x, scale.y, scale.z);
  glUniform1ui(1, 5); // Chunks Per Axis

  glPatchParameteri(GL_PATCH_VERTICES, 4);
  glDrawArrays(GL_PATCHES, 0, 4 * 5 * 5);
}
