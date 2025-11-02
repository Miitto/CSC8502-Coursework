#include "renderer.hpp"

#include "goober.hpp"
#include "heightmap.hpp"
#include "logger/logger.hpp"
#include <glm\ext\matrix_transform.hpp>
#include <imgui/imgui.h>

Renderer::Renderer(int width, int height, const char title[])
    : engine::App(width, height, title),
      camera(0.1f, 10000.0f, 4.0f / 3.0f, glm::radians(90.0f)),
      windowSize(window.size()) {

  auto copyProgOpt = gl::Program::fromFiles(
      {{SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "tex.frag.glsl", gl::Shader::Type::FRAGMENT}});

  auto meshDataOpt = engine::mesh::Data::fromFile(MESHDIR "OffsetCubeY.msh");
  if (!meshDataOpt) {
    Logger::error("Failed to load cube mesh: {}", meshDataOpt.error());
    bail();
    return;
  }

  auto heightmapResult = Heightmap::fromFile(TEXTUREDIR "terrain/height.png",
                                             TEXTUREDIR "terrain/diffuse.png");
  if (!heightmapResult) {
    Logger::error("Failed to load heightmap: {}", heightmapResult.error());
    bail();
    return;
  }

  std::shared_ptr<Heightmap> heightmap =
      std::make_shared<Heightmap>(std::move(heightmapResult.value()));

  graph.AddChild(heightmap);

  camera.SetPosition({0.f, 300.f, 0.0f});

  auto gooberResult = Goober::create(5);
  if (!gooberResult) {
    Logger::error("Failed to create Goober: {}", gooberResult.error());
    bail();
    return;
  }
  auto& goober = *gooberResult;
  goober[0].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(9.5f, 268.75f, 0.0f)));
  goober[1].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(9.5f, 268.75f, 10.0f)));
  goober[1].setFrame(10);
  goober[2].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(9.5f, 268.75f, 20.0f)));
  goober[2].setFrame(20);
  goober[3].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(9.5f, 268.75f, 30.0f)));
  goober[3].setFrame(30);
  goober[4].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(9.5f, 268.75f, 40.0f)));
  goober[4].setFrame(40);

  graph.AddChild(std::make_shared<Goober>(std::move(gooberResult.value())));

  auto copyPPOpt = PostProcess::create(SHADERDIR "tex.frag.glsl");
  if (!copyPPOpt) {
    Logger::error("Failed to create test post process: {}", copyPPOpt.error());
    bail();
    return;
  }

  copyPP = std::move(*copyPPOpt);

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
  auto& frustum = camera.GetFrustum();
  nodeLists.render(info, camera, frustum);

  if (!postProcesses.empty()) {
    auto bound = 0;
    glDisable(GL_DEPTH_TEST);

    auto flip = [&]() {
      postProcessFlipFlops[bound].tex.bind(0);
      postProcessFlipFlops[bound].depthTex.bind(1);
      bound = 1 - bound;
      postProcessFlipFlops[bound].fbo.bind();
    };

    for (size_t i = 0; i < postProcesses.size(); ++i) {
      auto& pp = postProcesses[i];
      flip();
      pp->run(flip);
    }
    gl::Framebuffer::unbind();
    postProcessFlipFlops[bound].tex.bind(0);

    copyPP.run(flip);
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
