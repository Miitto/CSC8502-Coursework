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

  auto copyProgOpt = gl::Program::fromFiles(
      {{SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "tex.frag.glsl", gl::Shader::Type::FRAGMENT}});

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

  auto testPP = PostProcess::create(SHADERDIR "tex.frag.glsl");
  if (!testPP) {
    Logger::error("Failed to create test post process: {}", testPP.error());
    bail();
    return;
  }

  std::unique_ptr<PostProcess> pp =
      std::make_unique<PostProcess>(std::move(testPP.value()));

  postProcesses.push_back(pp);

  setupPostProcesses(width, height);
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
    setupPostProcesses(windowSize.width, windowSize.height);
    camera.onResize(windowSize.width, windowSize.height);
  }

  auto nodeLists = graph.BuildNodeLists(camera);

  {
    engine::gui::GuiWindow cameraFrame("Camera");
    camera.CameraDebugUI();
    ImGui::Text("O: %d | T: %d", nodeLists.opaque.size(),
                nodeLists.transparent.size());
  }

  glEnable(GL_DEPTH_TEST);
  if (!postProcesses.empty()) {
    auto& fbo = postProcessFlipFlops[0].fbo;
    fbo.bind();
    const GLint clearColor[] = {0, 0, 0, 1};
    glClearNamedFramebufferiv(fbo.id(), GL_COLOR, 0, (const GLint*)&clearColor);
    glClearNamedFramebufferfi(fbo.id(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
  } else {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  nodeLists.render(info, camera);

  if (!postProcesses.empty()) {
    auto bound = 0;
    glDisable(GL_DEPTH_TEST);

    for (size_t i = 0; i < postProcesses.size(); ++i) {
      const auto& pp = postProcesses[i];
      const auto& src = postProcessFlipFlops[bound];
      const auto& dst = postProcessFlipFlops[1 - bound];
      if (i == postProcesses.size() - 1) {
        gl::Framebuffer::unbind();
      } else {
        dst.fbo.bind();
      }
      src.tex.bind(0);
      pp->run();
      bound = 1 - bound;
    }
  }
}

void Renderer::setupPostProcesses(int width, int height) {
  for (auto& pp : postProcessFlipFlops) {
    pp.tex.storage(1, GL_RGBA8, {width, height});
    pp.fbo.attachTexture(GL_COLOR_ATTACHMENT0, pp.tex);
    pp.depthTex.storage(1, GL_DEPTH24_STENCIL8, {width, height});
    pp.fbo.attachTexture(GL_DEPTH_STENCIL_ATTACHMENT, pp.depthTex);
  }
}
