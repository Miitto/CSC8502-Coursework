#include "renderer.hpp"

#include "goober.hpp"
#include "heightmap.hpp"
#include "logger/logger.hpp"
#include <gl/structs.hpp>
#include <glm\ext\matrix_transform.hpp>
#include <imgui/imgui.h>
#include <spdlog/fmt/bundled/format.h>

namespace {
  enum class DebugView {
    NONE,
    DIFFUSE,
    NORMAL,
    MATERIAL,
    DEPTH,
    DIFFUSE_LIGHT,
    SPECULAR_LIGHT
  };

  DebugView debugView = DebugView::NONE;
} // namespace

template <>
struct fmt::formatter<DebugView> : fmt::formatter<std::string_view> {
  auto format(DebugView c, fmt::format_context& ctx) const
      -> fmt::format_context::iterator {
    std::string_view name = "NONE";
    switch (c) {
    case DebugView::NONE:
      // NOOP
      break;
    case DebugView::DIFFUSE:
      name = "Diffuse";
      break;
    case DebugView::NORMAL:
      name = "Normal";
      break;
    case DebugView::MATERIAL:
      name = "Material";
      break;
    case DebugView::DEPTH:
      name = "Depth";
      break;
    case DebugView::DIFFUSE_LIGHT:
      name = "Diffuse Light";
      break;
    case DebugView::SPECULAR_LIGHT:
      name = "Specular Light";
      break;
    }

    return fmt::formatter<std::string_view>::format(name, ctx);
  }
};

Renderer::Renderer(int width, int height, const char title[])
    : engine::App(width, height, title),
      camera(0.1f, 10000.0f, 4.0f / 3.0f, glm::radians(90.0f)) {

  camera.onResize(width, height);

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
                                             TEXTUREDIR "terrain/diffuse.png",
                                             TEXTUREDIR "terrain/normal.png");
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
      glm::translate(glm::mat4(1.0f), glm::vec3(25.f, 268.75f, 0.0f)));
  goober[1].setFrame(10);
  goober[2].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(40.f, 268.75f, 0.0f)));
  goober[2].setFrame(20);
  goober[3].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(55.f, 268.75f, 0.0f)));
  goober[3].setFrame(30);
  goober[4].SetTransform(
      glm::translate(glm::mat4(1.0f), glm::vec3(70.f, 268.75f, 0.0f)));
  goober[4].setFrame(40);

  graph.AddChild(std::make_shared<Goober>(std::move(gooberResult.value())));

  auto copyPPOpt = PostProcess::create(SHADERDIR "tex.frag.glsl");
  if (!copyPPOpt) {
    Logger::error("Failed to create test post process: {}", copyPPOpt.error());
    bail();
    return;
  }
  copyPP = std::move(*copyPPOpt);

  auto depthViewOpt = gl::Program::fromFiles(
      {{SHADERDIR "fullscreen.vert.glsl", gl::Shader::VERTEX},
       {SHADERDIR "debug/depth.frag.glsl", gl::Shader::FRAGMENT}});
  if (!depthViewOpt) {
    Logger::error("Failed to create depth view post process: {}",
                  depthViewOpt.error());
    bail();
    return;
  }
  depthView = std::move(*depthViewOpt);

  auto pointLightOpt = gl::Program::fromFiles(
      {{SHADERDIR "lighting/point_light.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/point_light.frag.glsl",
        gl::Shader::Type::FRAGMENT}});
  if (!pointLightOpt) {
    Logger::error("Failed to create deferred light program: {}",
                  pointLightOpt.error());
    bail();
    return;
  }
  pointLight = std::move(*pointLightOpt);

  auto deferredLightCombineOpt = gl::Program::fromFiles(
      {{SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/combine.frag.glsl", gl::Shader::Type::FRAGMENT}});

  if (!deferredLightCombineOpt) {
    Logger::error("Failed to create deferred light combine program: {}",
                  deferredLightCombineOpt.error());
    bail();
    return;
  }
  deferredLightCombine = std::move(*deferredLightCombineOpt);

  pointLights.emplace_back(glm::vec3(0, 300, 0), glm::vec4(0.8, 0.8, 0.8, 1.0),
                           50.f);

  auto instanceSize =
      static_cast<GLuint>(pointLights.size()) * PointLight::dataSize();

  pointLightBuffer.init(instanceSize, nullptr,
                        gl::Buffer::Usage::DYNAMIC | gl::Buffer::Usage::WRITE |
                            gl::Buffer::Usage::PERSISTENT |
                            gl::Buffer::Usage::COHERENT);
  pointLightMapping = pointLightBuffer.map(gl::Buffer::Mapping::WRITE |
                                           gl::Buffer::Mapping::PERSISTENT |
                                           gl::Buffer::Mapping::COHERENT);

  PointLight::setupVao(pointLightVao, pointLightBuffer, 0);

  setupPostProcesses(width, height);
  setupLightFbo(width, height);
}

void Renderer::onWindowResize(engine::Window::Size newSize) {
  setupPostProcesses(newSize.width, newSize.height);
  setupLightFbo(newSize.width, newSize.height);
  camera.onResize(newSize.width, newSize.height);
}

void Renderer::update(const engine::FrameInfo& info) {
  engine::App::update(info);
  camera.update(input, info.frameDelta);

  graph.update(info);
}

void Renderer::render(const engine::FrameInfo& info) {
  gbuffers->fbo.bind();
  glDisable(GL_BLEND);
  glClearDepth(0.0f);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

  auto nodeLists = graph.BuildNodeLists(camera);

  {
    engine::gui::GuiWindow cameraFrame("Camera");
    camera.CameraDebugUI();
    ImGui::Text("O: %d | T: %d", nodeLists.opaque.size(),
                nodeLists.transparent.size());
    ImGui::SeparatorText("Debug Views");
    if (ImGui::BeginCombo("View", fmt::format("{}", debugView).c_str())) {
#define SEL(NAME, ENUM)                                                        \
  if (ImGui::Selectable(NAME, debugView == DebugView::ENUM)) {                 \
    debugView = DebugView::ENUM;                                               \
  }
      SEL("None", NONE);
      SEL("Diffuse", DIFFUSE);
      SEL("Normal", NORMAL);
      SEL("Material", MATERIAL);
      SEL("Depth", DEPTH);
      SEL("Diffuse Light", DIFFUSE_LIGHT);
      SEL("Specular Light", SPECULAR_LIGHT);
#undef SEL

      ImGui::EndCombo();
    }
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  auto& frustum = camera.GetFrustum();
  nodeLists.renderLit(info, camera, frustum);

  if (debugView == DebugView::DIFFUSE || debugView == DebugView::NORMAL ||
      debugView == DebugView::MATERIAL || debugView == DebugView::DEPTH) {
    gl::Framebuffer::unbind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    switch (debugView) {
    case DebugView::DIFFUSE:
      gbuffers->diffuse.bind(0);
      break;
    case DebugView::NORMAL:
      gbuffers->normal.bind(0);
      break;
    case DebugView::MATERIAL:
      gbuffers->material.bind(0);
      break;
    case DebugView::DEPTH:
      depthView.bind();
      glUniform1f(1, camera.getNear());
      glUniform1f(0, camera.getFar());
      gbuffers->depthStencil.bind(0);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      return;
    default:
      break;
    }
    auto bg = dummyVao.bindGuard();
    copyPP.run([]() {});
    return;
  }

  if (!renderPointLights())
    return;
  auto bg = dummyVao.bindGuard();
  if (!combineDeferredLightBuffers())
    return;
  renderPostProcesses();
}

bool Renderer::renderPointLights() {
  pointLight.bind();

  gbuffers->diffuse.bind(0);
  gbuffers->normal.bind(1);
  gbuffers->material.bind(2);
  gbuffers->depthStencil.bind(3);

  glDisable(GL_DEPTH_TEST);
  glCullFace(GL_FRONT);

  lightFbo.fbo.bind();
  auto bg = pointLightVao.bindGuard();

  GLuint offset = 0;
  for (auto& pl : pointLights) {
    pl.writeInstanceData({pointLightMapping, offset});
    offset += PointLight::dataSize();
  }

  glDrawArrays(GL_TRIANGLES, 0, 1);

  if (debugView == DebugView::DIFFUSE_LIGHT ||
      debugView == DebugView::SPECULAR_LIGHT) {
    gl::Framebuffer::unbind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    switch (debugView) {
    case DebugView::DIFFUSE_LIGHT:
      lightFbo.diffuse.bind(0);
      break;
    case DebugView::SPECULAR_LIGHT:
      lightFbo.specular.bind(0);
      break;
    default:
      break;
    }
    copyPP.run([]() {});

    return false;
  }

  return true;
}

bool Renderer::combineDeferredLightBuffers() {
  glDisable(GL_CULL_FACE);
  deferredLightCombine.bind();
  postProcessFlipFlops[0].fbo.bind();
  gbuffers->diffuse.bind(0);
  lightFbo.diffuse.bind(1);
  lightFbo.specular.bind(2);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  return true;
}

void Renderer::renderPostProcesses() {
  auto bound = 0;

  auto flip = [&]() {
    postProcessFlipFlops[bound].tex.bind(0);
    bound = 1 - bound;
    postProcessFlipFlops[bound].fbo.bind();
  };

  gbuffers->depthStencil.bind(1);

  for (size_t i = 0; i < postProcesses.size(); ++i) {
    auto& pp = postProcesses[i];
    flip();
    pp->run(flip);
  }
  gl::Framebuffer::unbind();
  postProcessFlipFlops[bound].fbo.blit(
      0, 0, 0, windowSize.width, windowSize.height, 0, 0, windowSize.width,
      windowSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::setupPostProcesses(int width, int height) {
  for (auto& pp : postProcessFlipFlops) {
    pp.tex = {};
    pp.tex.storage(1, GL_RGBA8, {width, height});
    pp.fbo = {};
    pp.fbo.attachTexture(GL_COLOR_ATTACHMENT0, pp.tex);
  }
}

void Renderer::setupLightFbo(int width, int height) {
  lightFbo.diffuse = {};
  lightFbo.diffuse.storage(1, GL_RGBA8, {width, height});
  lightFbo.specular = {};
  lightFbo.specular.storage(1, GL_RGBA8, {width, height});
  lightFbo.fbo = {};
  lightFbo.fbo.attachTexture(GL_COLOR_ATTACHMENT0, lightFbo.diffuse);
  lightFbo.fbo.attachTexture(GL_COLOR_ATTACHMENT1, lightFbo.specular);

  constexpr GLenum attachments[2] = {GL_COLOR_ATTACHMENT0,
                                     GL_COLOR_ATTACHMENT1};
  glNamedFramebufferDrawBuffers(lightFbo.fbo.id(), 2, attachments);
}
