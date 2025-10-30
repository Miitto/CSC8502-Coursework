#include "renderer.hpp"

#include "Robot.h"
#include "heightmap.hpp"
#include "logger/logger.hpp"
#include <imgui/imgui.h>

Renderer::Renderer(int width, int height, const char title[])
    : engine::App(width, height, title),
      camera(0.1f, 10000.0f, 4.0f / 3.0f, glm::radians(90.0f)) {

  cubeMesh = std::shared_ptr<engine::Mesh>(
      engine::Mesh::LoadFromMeshFile(MESHDIR "OffsetCubeY.msh"));
  if (!cubeMesh) {
    Logger::error("Failed to load cube mesh!");
    bail();
    return;
  }

  auto heightmapResult = Heightmap::fromFile(TEXTUREDIR "noise.png");
  if (!heightmapResult) {
    Logger::error("Failed to load heightmap: {}", heightmapResult.error());
    bail();
    return;
  }

  std::shared_ptr<Heightmap> heightmap =
      std::make_shared<Heightmap>(std::move(heightmapResult.value()));

  graph.AddChild(heightmap);

  camera.SetPosition({0.f, 30.f, 175.f});

  graph.AddChild(std::make_shared<Robot>(cubeMesh));
}

void Renderer::update(const engine::FrameInfo& info) {
  engine::App::update(info);
  camera.update(input, info.frameDelta);

  graph.update(info);
}

void Renderer::render(const engine::FrameInfo& info) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);

  auto nodeLists = graph.BuildNodeLists(camera);

  {
    engine::gui::GuiWindow cameraFrame("Camera");
    camera.CameraDebugUI();
    ImGui::Text("O: %d | T: %d", nodeLists.opaque.size(),
                nodeLists.transparent.size());
  }

  nodeLists.render(info, camera);
}
