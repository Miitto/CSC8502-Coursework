#pragma once

#include "postprocess.hpp"

class Blur : public PostProcess {
  Blur(gl::Program&& program) : PostProcess(std::move(program), "Blur") {}

public:
  Blur() = default;
  inline static std::expected<Blur, std::string> create() {
    auto programOpt = gl::Program::fromFiles({
        {SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
        {SHADERDIR "postprocess/blur.frag.glsl", gl::Shader::Type::FRAGMENT},
    });
    if (!programOpt) {
      return std::unexpected(programOpt.error());
    }

    return Blur(std::move(programOpt.value()));
  }

  void run(std::function<void()> flip) const override {
    (void)flip;
    program.bind();

    glUniform1i(0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    flip();
    glUniform1i(0, 1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
};