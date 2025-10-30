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

  return Heightmap(std::move(tex), std::move(prog));
}

void Heightmap::render(const engine::Camera& camera) {
  program.bind();
  texture.bind(0);

  camera.bindMatrixBuffer(0);

  {
    engine::gui::GuiWindow frame("Heightmap");
    ImGui::InputFloat3("Scale", &scale.x);
  }

  glUniform3f(0, scale.x, scale.y, scale.z);

  glPatchParameteri(GL_PATCH_VERTICES, 4);
  glDrawArrays(GL_PATCHES, 0, 4);
}
