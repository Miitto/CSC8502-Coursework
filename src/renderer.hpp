#pragma once

#include "postprocess.hpp"
#include <engine/app.hpp>
#include <engine/camera.hpp>
#include <engine/mesh.hpp>
#include <gl/gl.hpp>
#include <memory>

class Renderer : public engine::App {
public:
  Renderer(int width, int height, const char title[]);

  void update(const engine::FrameInfo& frame) override;
  void render(const engine::FrameInfo& frame) override;

private:
  engine::PerspectiveCamera camera;

  std::shared_ptr<engine::Mesh> cubeMesh;
  engine::scene::Graph graph;
  std::vector<PostProcess> postProcesses;

  engine::Window::Size windowSize;

  gl::Texture fboTex;
  gl::Framebuffer fbo;
};