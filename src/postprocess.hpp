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

  virtual ~PostProcess() = default;

  /// <summary>
  /// Runs the post-process effect.
  /// The current render target color texture will be bound at unit 0.
  /// The current render target depth texture will be bound at unit 1.
  /// Depth testing is disabled.
  /// </summary>
  /// <param name="flip">Flips the render target if the effect needs to
  /// ping-pong. Should be left on the last target you rendered to</param>
  virtual void run(std::function<void()> flip) const {
    (void)flip;
    program.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

protected:
  PostProcess(gl::Program&& program) : program(std::move(program)) {}

  gl::Program program;
};