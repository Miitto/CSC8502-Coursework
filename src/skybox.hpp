#pragma once

#include "postprocess.hpp"
#include <gl/gl.hpp>

class Skybox : public PostProcess {
  Skybox(const gl::CubeMap& cubeMap, gl::Program&& program)
      : PostProcess(std::move(program)), cubeMap(cubeMap) {}

public:
  Skybox() = delete;
  Skybox(const Skybox&) = delete;
  Skybox& operator=(const Skybox&) = delete;
  Skybox(Skybox&&) noexcept = default;
  Skybox& operator=(Skybox&&) noexcept = default;

  inline static std::expected<Skybox, std::string>
  create(const gl::CubeMap& cubeMap) {
    auto programOpt = gl::Program::fromFiles({
        {SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
        {SHADERDIR "skybox.frag.glsl", gl::Shader::Type::FRAGMENT},
    });
    if (!programOpt.has_value()) {
      return std::unexpected(programOpt.error());
    }
    return Skybox(cubeMap, std::move(*programOpt));
  }

  void run(std::function<void()> flip) const override {
    (void)flip;
    cubeMap.bind(2);
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

protected:
  const gl::CubeMap& cubeMap;
};