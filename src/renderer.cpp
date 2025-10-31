#include "renderer.hpp"

#include "Robot.h"
#include "goober.hpp"
#include "heightmap.hpp"
#include "logger/logger.hpp"
#include <imgui/imgui.h>

Renderer::Renderer(int width, int height, const char title[])
    : engine::App(width, height, title),
      camera(0.1f, 10000.0f, 4.0f / 3.0f, glm::radians(90.0f)),
      windowSize(window.size()) {

  auto meshOpt = engine::Mesh::LoadFromMeshFile(MESHDIR "OffsetCubeY.msh");
  if (!meshOpt) {
    Logger::error("Failed to load cube mesh: {}", meshOpt.error());
    bail();
    return;
  }

  cubeMesh = std::make_shared<engine::Mesh>(std::move(*meshOpt));

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

  auto gooberResult = Goober::create();
  if (!gooberResult) {
    Logger::error("Failed to create Goober: {}", gooberResult.error());
    bail();
    return;
  }
  graph.AddChild(std::make_shared<Goober>(std::move(gooberResult.value())));

  fboTex.storage(1, GL_RGBA8, {width, height});
  fbo.attachTexture(GL_COLOR_ATTACHMENT0, fboTex);
}

void Renderer::update(const engine::FrameInfo& info) {
  engine::App::update(info);
  camera.update(input, info.frameDelta);

  graph.update(info);
}

void Renderer::render(const engine::FrameInfo& info) {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  if (window.size() != windowSize) {
    windowSize = window.size();
    fboTex = gl::Texture();
    fboTex.storage(1, GL_RGBA8, {windowSize.width, windowSize.height});
    fbo = gl::Framebuffer();
    fbo.attachTexture(GL_COLOR_ATTACHMENT0, fboTex);
    camera.onResize(windowSize.width, windowSize.height);
  }

  auto nodeLists = graph.BuildNodeLists(camera);

  {
    engine::gui::GuiWindow cameraFrame("Camera");
    camera.CameraDebugUI();
    ImGui::Text("O: %d | T: %d", nodeLists.opaque.size(),
                nodeLists.transparent.size());
  }

  fbo.bind();
  glEnable(GL_DEPTH_TEST);
  const GLint clearColor[] = {0, 0, 0, 1};
  glClearNamedFramebufferiv(fbo.id(), GL_COLOR, 0, (const GLint*)&clearColor);
  glClearNamedFramebufferfi(fbo.id(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
  nodeLists.render(info, camera);
  fbo.unbind();

  glDisable(GL_DEPTH_TEST);
  // TODO: Post-process
}
