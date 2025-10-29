#pragma once

#include <engine/app.hpp>
#include <engine/camera.hpp>
#include <engine/mesh.hpp>
#include <gl/gl.hpp>
#include <memory>

class Renderer : public engine::App {
public:
  Renderer(int width, int height, const char title[]);

  void update(float dt) override;
  void render() override;

private:
  engine::PerspectiveCamera camera;

  std::shared_ptr<engine::Mesh> cubeMesh;
  engine::scene::Graph graph;
};