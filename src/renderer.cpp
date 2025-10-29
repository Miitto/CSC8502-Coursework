#include "renderer.hpp"

#include "Robot.h"
#include "logger/logger.hpp"
#include <filesystem>

Renderer::Renderer(int width, int height, const char title[])
    : engine::App(width, height, title),
      camera(0.1f, 1000.0f, 4.0f / 3.0f, glm::radians(90.0f)) {

  cubeMesh = std::shared_ptr<engine::Mesh>(
      engine::Mesh::LoadFromMeshFile(MESHDIR "OffsetCubeY.msh"));
  if (!cubeMesh) {
    Logger::error("Failed to load cube mesh!");
    bail();
    return;
  }

  camera.SetPosition({0.f, 30.f, 175.f});

  graph.AddChild(std::make_shared<Robot>(cubeMesh));
}

void Renderer::update(float dt) {
  engine::App::update(dt);
  camera.update(input, dt);

  graph.update(dt);
}

void Renderer::render() {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  {
    engine::gui::GuiWindow cameraFrame("Camera");
    camera.CameraDebugUI();
  }

  graph.render(camera);
}
