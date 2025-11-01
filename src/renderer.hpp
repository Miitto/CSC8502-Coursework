#pragma once

#include "postprocess.hpp"
#include <engine/app.hpp>
#include <engine/camera.hpp>
#include <engine/mesh/mesh.hpp>
#include <gl/gl.hpp>
#include <memory>

class Renderer : public engine::App {
public:
  Renderer(int width, int height, const char title[]);

  void update(const engine::FrameInfo& frame) override;
  void render(const engine::FrameInfo& frame) override;

private:
  engine::PerspectiveCamera camera;

  gl::Buffer cubeMeshBuffer = {};
  std::shared_ptr<engine::Mesh> cubeMesh;
  engine::scene::Graph graph;
  std::vector<std::unique_ptr<PostProcess>> postProcesses;
  gl::Program copyProgram;

  engine::Window::Size windowSize;

  struct Fbos {
    gl::Texture tex = {};
    gl::Texture depthTex = {};
    gl::Framebuffer fbo = {};
  };

  std::array<Fbos, 2> postProcessFlipFlops = {};

  void setupPostProcesses(int width, int height);
};