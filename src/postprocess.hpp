#pragma once

#include <expected>
#include <gl/gl.hpp>

class PostProcess {
public:
  static std::expected<PostProcess, std::string> create(std::string_view file);

public:
  PostProcess() = default;
  PostProcess(const PostProcess&) = delete;
  PostProcess& operator=(const PostProcess&) = delete;
  PostProcess(PostProcess&&) noexcept = default;
  PostProcess& operator=(PostProcess&&) noexcept = default;

  inline void run() const {
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

protected:
  PostProcess(gl::Program&& program) : program(std::move(program)) {}

  gl::Program program;
};