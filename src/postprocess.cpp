#include "postprocess.hpp"

std::expected<PostProcess, std::string>
PostProcess::create(std::string_view file) {
  auto programOpt = gl::Program::fromFiles({
      {SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
      {file, gl::Shader::Type::FRAGMENT},
  });
  if (!programOpt) {
    return std::unexpected(programOpt.error());
  }

  return PostProcess(std::move(programOpt.value()), file);
}