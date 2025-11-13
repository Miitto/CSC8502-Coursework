#include "renderer.hpp"

#include "heightmap.hpp"
#include "logger/logger.hpp"
#include "skybox.hpp"
#include "water.hpp"
#include <engine/globals.hpp>
#include <engine/mesh_node.hpp>
#include <engine\mesh\mesh.hpp>
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
    DIFFUSE_LIGHT,
    SPECULAR_LIGHT
  };

  DebugView debugView = DebugView::NONE;

  GLuint writeLitDraws(const engine::scene::Graph::NodeLists& nodeLists,
                       gl::MappingRef& mapping) {
    GLuint writtenDraws = 0;
    for (const auto& child : nodeLists.lit) {
      child.node->writeBatchedDraws(mapping, writtenDraws);
    }

    return writtenDraws;
  }

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

void Renderer::onWindowResize(engine::Window::Size newSize) {
  App::onWindowResize(newSize);
  camera.onResize(newSize.width, newSize.height);
  setupHdrOutput(newSize.width, newSize.height);
  setupPostProcesses(newSize.width, newSize.height);
  setupLightFbo(newSize.width, newSize.height);
}

bool Renderer::update(const engine::FrameInfo& info) {
  if (engine::App::update(info))
    return true;

  if (onTrack) {
    track.update(info.frameDelta);
    auto pos = track.position();
    auto rot = track.rotation();

    auto& cam = camera.getSplitRatio() < 1.0f ? this->camera.left()
                                              : this->camera.right();
    cam.SetPosition(pos);
    cam.SetRotation(rot);

    if (input.isKeyDown(GLFW_KEY_ESCAPE)) {
      onTrack = false;
      camera.left().EnableMouse(true);
    }
  }

  camera.update(input, info.frameDelta, !onTrack);

  if (input.isKeyPressed(GLFW_KEY_B))
    enableBloom = !enableBloom;

  if (input.isKeyPressed(GLFW_KEY_U))
    enableDebugUi = !enableDebugUi;

  if (input.isKeyPressed(GLFW_KEY_F11)) {
    window.fullscreen(!window.isFullscreen());
  }

  graph.update(info);

  return false;
}

void Renderer::useLeftCamera() {
  camera.leftView();
  camera.left().bindMatrixBuffer(0);
  envMap.bind(4);
}

void Renderer::useRightCamera() {
  camera.rightView();
  camera.right().bindMatrixBuffer(0);
  nightEnvMap.bind(4);
}

void Renderer::render(const engine::FrameInfo& info) {
  (void)info;
  gbuffers->fbo.bind();
  glClearDepth(0.0f);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto batch = setupBatches();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glClear(GL_DEPTH_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glDisable(GL_BLEND);

  if (camera.getSplitRatio() < 1.0f) {
    useLeftCamera();
    auto& camera = this->camera.left();
    auto nodeLists =
        graph.BuildNodeLists(camera.GetFrustum(), camera.GetPosition());

    renderLit(nodeLists, batch, camera, 0);
  }

  if (camera.getSplitRatio() > 0.0f) {
    useRightCamera();
    auto& camera = this->camera.right();
    auto nodeLists =
        rightGraph.BuildNodeLists(camera.GetFrustum(), camera.GetPosition());
    renderLit(nodeLists, batch, camera, rightIndirectOffset);
  }

  camera.fullView();

  if (enableDebugUi)
    debugUi(info);

  if (debugView == DebugView::DIFFUSE || debugView == DebugView::NORMAL ||
      debugView == DebugView::MATERIAL) {
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
    default:
      break;
    }
    auto bg = engine::globals::DUMMY_VAO.bindGuard();
    copyPP.run([]() {});
    return;
  }

  renderPointLights();
  renderSpotLights();

  if (debugView == DebugView::DIFFUSE_LIGHT ||
      debugView == DebugView::SPECULAR_LIGHT) {
    gl::Framebuffer::unbind();
    auto bg = engine::globals::DUMMY_VAO.bindGuard();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
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
    return;
  }

  auto bg = engine::globals::DUMMY_VAO.bindGuard();
  if (!combineDeferredLightBuffers())
    return;

  renderPostProcesses();
}

Renderer::BatchSetup Renderer::setupBatches() {
  auto& leftRoots = graph.GetRoots();
  auto& rightRoots = rightGraph.GetRoots();

  engine::scene::Node::DrawParams leftDrawParams = {0, 0, 0};
  for (const auto& node : leftRoots) {
    leftDrawParams += node->getBatchDrawParams();
  }

  engine::scene::Node::DrawParams rightDrawParams = {0, 0, 0};
  for (const auto& node : rightRoots) {
    rightDrawParams += node->getBatchDrawParams();
  }

  engine::scene::Node::DrawParams drawParams = leftDrawParams + rightDrawParams;

  GLuint verticesSize = drawParams.maxVertices * sizeof(engine::mesh::Vertex);
  if (verticesSize > skinnedVerticesBuffer.size()) {
    skinnedVerticesBuffer = {};
    skinnedVerticesBuffer.label("Skinned Vertices Buffer");
    skinnedVerticesBuffer.init(verticesSize);
    batchVao.bindVertexBuffer(0, skinnedVerticesBuffer.id(), 0,
                              sizeof(engine::mesh::Vertex));
  }

  skinProgram.bind();

  staticBuffer.bindRange(gl::Buffer::StorageTarget::STORAGE, 1, 0,
                         staticVertexSize);
  skinnedVerticesBuffer.bindRange(gl::Buffer::StorageTarget::STORAGE, 2, 0,
                                  verticesSize);
  staticBuffer.bindRange(gl::Buffer::StorageTarget::STORAGE, 3, jointOffset,
                         jointSize);
  {
    GLuint writtenVertices = 0;
    for (const auto& node : leftRoots) {
      node->skinVertices(writtenVertices);
    }
    for (const auto& node : rightRoots) {
      node->skinVertices(writtenVertices);
    }
  }

  auto indirectSize = static_cast<GLuint>(
      drawParams.maxIndirectCmds * sizeof(gl::DrawElementsIndirectCommand));
  rightIndirectOffset =
      leftDrawParams.maxIndirectCmds * sizeof(gl::DrawElementsIndirectCommand);
  auto instanceSize =
      static_cast<GLuint>(drawParams.instances * sizeof(glm::mat4));
  auto textureSize = static_cast<GLuint>(
      drawParams.maxIndirectCmds * sizeof(engine::mesh::TextureHandleSet));
  auto textureOffset = gl::Buffer::roundToAlignment(
      indirectSize + instanceSize, gl::UNIFORM_BUFFER_OFFSET_ALIGNMENT);

  auto dynamicSize = textureOffset + textureSize;
  if (dynamicBuffer.size() < dynamicSize) {
    Logger::debug("Resizing dynamic buffer from {} to {}. Instance Offset: {} "
                  "| Texture Offset: {}",
                  dynamicBuffer.size(), dynamicSize, indirectSize,
                  textureOffset);
    dynamicBuffer = {};
    dynamicBuffer.label("Dynamic Buffer");
    dynamicBuffer.init(dynamicSize, nullptr,
                       gl::Buffer::Usage::WRITE |
                           gl::Buffer::Usage::PERSISTENT |
                           gl::Buffer::Usage::COHERENT);
    dynamicMapping = dynamicBuffer.map(gl::Buffer::Mapping::WRITE |
                                       gl::Buffer::Mapping::PERSISTENT |
                                       gl::Buffer::Mapping::COHERENT);
    batchVao.bindVertexBuffer(1, dynamicBuffer.id(), indirectSize,
                              sizeof(glm::mat4));
  }

  GLuint writtenInstances = 0;
  gl::MappingRef indirectMap = {dynamicMapping, 0};
  gl::MappingRef instanceMap = {dynamicMapping,
                                static_cast<GLuint>(indirectSize)};
  gl::MappingRef textureMap = {dynamicMapping,
                               static_cast<GLuint>(textureOffset)};
  for (auto& node : leftRoots) {
    node->writeInstanceData(instanceMap, writtenInstances, textureMap);
  }
  for (auto& node : rightRoots) {
    node->writeInstanceData(instanceMap, writtenInstances, textureMap);
  }

  return {
      .textureOffset = textureOffset,
      .textureSize = textureSize,
  };
}

void Renderer::renderLit(const engine::scene::Graph::NodeLists& nodeLists,
                         const BatchSetup& batch, const engine::Camera& camera,
                         GLuint offset) {

  // Will likely be the larger more custom stuff like terrain
  nodeLists.renderLit(camera.GetFrustum());

  auto bg = batchVao.bindGuard();
  batchProgram.bind();
  gl::MappingRef indirectMap = {dynamicMapping, offset};
  auto draws = writeLitDraws(nodeLists, indirectMap);
  dynamicBuffer.bind(gl::Buffer::BasicTarget::DRAW_INDIRECT);

  dynamicBuffer.bindRange(gl::Buffer::StorageTarget::STORAGE, 2,
                          batch.textureOffset, batch.textureSize);

  glMultiDrawElementsIndirect(
      GL_TRIANGLES, GL_UNSIGNED_INT,
      reinterpret_cast<void*>(static_cast<uintptr_t>(offset)), draws,
      sizeof(gl::DrawElementsIndirectCommand));
}

void Renderer::debugUi(const engine::FrameInfo& frame) {
  (void)frame;

  engine::gui::GuiWindow cameraFrame("Camera");
  float splitRatio = camera.getSplitRatio();
  if (ImGui::SliderFloat("Split Ratio", &splitRatio, 0.0, 1.0)) {
    camera.setSplitRatio(splitRatio);
  }
  camera.left().CameraDebugUI();
  camera.right().CameraDebugUI();
  static bool vSync = true;
  if (ImGui::Checkbox("VSync", &vSync)) {
    int interval = vSync ? 1 : 0;
    glfwSwapInterval(interval);
    Logger::info("VSync {}", vSync ? "Enabled" : "Disabled");
  }

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
    SEL("Diffuse Light", DIFFUSE_LIGHT);
    SEL("Specular Light", SPECULAR_LIGHT);
#undef SEL

    ImGui::EndCombo();
  }
  ImGui::SeparatorText("Effects");
  ImGui::Checkbox("Enable Bloom", &enableBloom);

  ImGui::SeparatorText("Post Processes");
  for (auto& pp : postProcesses) {
    bool enabled = pp->isEnabled();
    if (ImGui::Checkbox(pp->name().data(), &enabled)) {
      pp->setEnabled(enabled);
    }
  }
}

void Renderer::renderPointLights() {
  glViewport(0, 0, PointLight::SHADOW_MAP_SIZE, PointLight::SHADOW_MAP_SIZE);
  dynamicBuffer.bind(gl::Buffer::BasicTarget::DRAW_INDIRECT);

  glCullFace(GL_FRONT);

  if (camera.getSplitRatio() < 1.0f) {
    GLuint writtenDraws = 0;
    gl::MappingRef indirectMap = {dynamicMapping, 0};
    for (const auto& root : graph.GetRoots()) {
      root->writeBatchedDraws(indirectMap, writtenDraws);
    }

    size_t idx = 0;
    auto renderFn = [&]() {
      shadowMatrixBuffers[idx].buffer.bindBase(
          gl::Buffer::StorageTarget::UNIFORM, 5);
      for (const auto& root : graph.GetRoots()) {
        root->renderDepthOnlyCube();
      }

      batchVao.bind();
      batchShadowCubeProgram.bind();

      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                  writtenDraws,
                                  sizeof(gl::DrawElementsIndirectCommand));
      ++idx;
    };

    for (size_t i = 0; i < pointLights.size(); ++i) {
      pointLights[i].renderShadowMap(renderFn, shadowMatrixBuffers[i].mapping);
    }
  }

  if (camera.getSplitRatio() > 0.0f) {
    GLuint writtenDraws = 0;
    gl::MappingRef indirectMap = {dynamicMapping, rightIndirectOffset};
    for (const auto& root : rightGraph.GetRoots()) {
      root->writeBatchedDraws(indirectMap, writtenDraws);
    }

    size_t idx = 0;
    auto renderFn = [&]() {
      shadowMatrixBuffers[idx + pointLights.size()].buffer.bindBase(
          gl::Buffer::StorageTarget::UNIFORM, 5);
      for (const auto& root : rightGraph.GetRoots()) {
        root->renderDepthOnlyCube();
      }

      batchVao.bind();
      batchShadowCubeProgram.bind();

      glMultiDrawElementsIndirect(
          GL_TRIANGLES, GL_UNSIGNED_INT,
          reinterpret_cast<void*>(static_cast<uintptr_t>(rightIndirectOffset)),
          writtenDraws, sizeof(gl::DrawElementsIndirectCommand));
      ++idx;
    };

    for (size_t i = 0; i < rightPointLights.size(); ++i) {
      rightPointLights[i].renderShadowMap(
          renderFn, shadowMatrixBuffers[i + pointLights.size()].mapping);
    }
  }

  gl::Vao::unbind();

  pointLight.bind();

  lightFbo.fbo.bind();

  constexpr glm::vec4 clearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearNamedFramebufferfv(lightFbo.fbo.id(), GL_COLOR, 0, &clearColor.x);
  glClearNamedFramebufferfv(lightFbo.fbo.id(), GL_COLOR, 1, &clearColor.x);

  gbuffers->diffuse.bind(0);
  gbuffers->normal.bind(1);
  gbuffers->material.bind(2);
  gbuffers->depthStencil.bind(3);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  glUniform1ui(3, 0);

  auto bg = pointLightMesh.bindGuard();
  if (camera.getSplitRatio() < 1.0f) {
    useLeftCamera();
    for (size_t i = 0; i < pointLights.size(); ++i) {
      glUniform3fv(0, 1, &pointLights[i].position()[0]);
      glUniform1f(1, pointLights[i].radius());
      glUniform4fv(2, 1, &pointLights[i].color()[0]);
      pointLights[i].getShadowMap().bind(4);
      pointLightMesh.draw();
    }
  }
  if (camera.getSplitRatio() > 0.0f) {
    useRightCamera();
    for (size_t i = 0; i < rightPointLights.size(); ++i) {
      glUniform3fv(0, 1, &rightPointLights[i].position()[0]);
      glUniform1f(1, rightPointLights[i].radius());
      glUniform4fv(2, 1, &rightPointLights[i].color()[0]);
      rightPointLights[i].getShadowMap().bind(4);
      pointLightMesh.draw();
    }
  }
  camera.fullView();
}

void Renderer::renderSpotLights() {
  glViewport(0, 0, SpotLight::SHADOW_MAP_SIZE, SpotLight::SHADOW_MAP_SIZE);
  dynamicBuffer.bind(gl::Buffer::BasicTarget::DRAW_INDIRECT);

  glCullFace(GL_FRONT);
  glEnable(GL_DEPTH_TEST);

  if (camera.getSplitRatio() < 1.0f && !spotLights.empty()) {

    size_t idx = 0;
    auto renderFn = [&](const engine::Frustum& frustum,
                        const glm::vec3& position) {
      auto nodeLists = graph.BuildNodeLists(frustum, position);

      GLuint writtenDraws = 0;
      gl::MappingRef indirectMap = {dynamicMapping, 0};
      for (const auto& root : nodeLists.lit) {
        root.node->writeBatchedDraws(indirectMap, writtenDraws);
      }

      spotShadowMatrixBuffers[idx].buffer.bindBase(
          gl::Buffer::StorageTarget::UNIFORM, 5);
      for (const auto& root : nodeLists.lit) {
        root.node->renderDepthOnly(frustum);
      }

      batchVao.bind();
      batchShadowProgram.bind();

      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                  writtenDraws,
                                  sizeof(gl::DrawElementsIndirectCommand));
      ++idx;
    };

    for (size_t i = 0; i < spotLights.size(); ++i) {
      spotLights[i].renderShadowMap(renderFn,
                                    spotShadowMatrixBuffers[i].mapping);
    }
  }

  if (camera.getSplitRatio() > 0.0f && !rightSpotLights.empty()) {
    size_t idx = 0;
    auto renderFn = [&](const engine::Frustum& frustum,
                        const glm::vec3& position) {
      auto nodeLists = rightGraph.BuildNodeLists(frustum, position);

      GLuint writtenDraws = 0;
      gl::MappingRef indirectMap = {dynamicMapping, rightIndirectOffset};
      for (const auto& root : nodeLists.lit) {
        root.node->writeBatchedDraws(indirectMap, writtenDraws);
      }

      spotShadowMatrixBuffers[idx + spotLights.size()].buffer.bindBase(
          gl::Buffer::StorageTarget::UNIFORM, 5);
      for (const auto& root : nodeLists.lit) {
        root.node->renderDepthOnly(frustum);
      }

      batchVao.bind();
      batchShadowProgram.bind();

      glMultiDrawElementsIndirect(
          GL_TRIANGLES, GL_UNSIGNED_INT,
          reinterpret_cast<void*>(static_cast<uintptr_t>(rightIndirectOffset)),
          writtenDraws, sizeof(gl::DrawElementsIndirectCommand));
      ++idx;
    };

    for (size_t i = 0; i < rightSpotLights.size(); ++i) {
      rightSpotLights[i].renderShadowMap(
          renderFn, spotShadowMatrixBuffers[i + spotLights.size()].mapping);
    }
  }

  gl::Vao::unbind();

  spotLight.bind();
  lightFbo.fbo.bind();

  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  auto bg = spotLightMesh.bindGuard();
  if (camera.getSplitRatio() < 1.0f) {
    useLeftCamera();
    for (size_t i = 0; i < spotLights.size(); ++i) {
      glUniform3fv(0, 1, &spotLights[i].position()[0]);
      glUniform1f(1, spotLights[i].radius());
      glUniform4fv(2, 1, &spotLights[i].color()[0]);
      glUniform3fv(3, 1, &spotLights[i].direction()[0]);
      auto modelMatrix = spotLights[i].modelMatrix();
      glUniformMatrix4fv(4, 1, GL_FALSE, &modelMatrix[0].x);
      spotLights[i].getShadowMap().bind(4);
      spotLightMesh.draw();
    }
  }
  if (camera.getSplitRatio() > 0.0f) {
    useRightCamera();
    for (size_t i = 0; i < rightSpotLights.size(); ++i) {
      glUniform3fv(0, 1, &rightSpotLights[i].position()[0]);
      glUniform1f(1, rightSpotLights[i].radius());
      glUniform4fv(2, 1, &rightSpotLights[i].color()[0]);
      glUniform3fv(3, 1, &rightSpotLights[i].direction()[0]);
      auto modelMatrix = rightSpotLights[i].modelMatrix();
      glUniformMatrix4fv(4, 1, GL_FALSE, &modelMatrix[0].x);
      rightSpotLights[i].getShadowMap().bind(4);
      spotLightMesh.draw();
    }
  }
  camera.fullView();
}

bool Renderer::combineDeferredLightBuffers() {
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  static glm::vec3 ambientLight(0.1f, 0.1f, 0.1f);

  if (enableDebugUi) {
    auto frame = engine::gui::GuiWindow("Lighting");
    ImGui::ColorEdit3("Ambient Light", &ambientLight.x);
  }

  deferredLightCombine.bind();
  hdrOutput.fbo.bind();
  gbuffers->diffuse.bind(0);
  lightFbo.diffuse.bind(1);
  lightFbo.specular.bind(2);

  glUniform3fv(0, 1, &ambientLight.x);
  glUniform1i(1, enableBloom ? 0 : 1);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  return true;
}

void Renderer::renderPostProcesses() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  auto bound = 0;

  auto flip = [&]() {
    postProcessFlipFlops[bound].tex.bind(0);
    bound = 1 - bound;
    postProcessFlipFlops[bound].fbo.bind();
  };

  if (enableBloom) {
    flip();
    bloomBrightTex.bind(0);
    blurPP.run(flip);
    flip();
    hdrOutput.tex.bind(1);
    bloomPP.run(flip);
  } else {
    hdrOutput.fbo.blit(postProcessFlipFlops[0].fbo.id(), 0, 0, windowSize.width,
                       windowSize.height, 0, 0, windowSize.width,
                       windowSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }

  gbuffers->normal.bind(1);
  gbuffers->material.bind(2);
  gbuffers->depthStencil.bind(3);
  lightFbo.diffuse.bind(5);
  lightFbo.specular.bind(6);

  for (size_t i = 0; i < postProcesses.size(); ++i) {
    auto& pp = postProcesses[i];
    if (!pp->isEnabled())
      continue;
    flip();

    if (camera.getSplitRatio() < 1.0f) {
      useLeftCamera();
      pp->run(flip);
    }
    if (camera.getSplitRatio() > 0.0f) {
      useRightCamera();
      pp->run(flip);
    }
  }
  gl::Framebuffer::unbind();
  postProcessFlipFlops[bound].fbo.blit(
      0, 0, 0, windowSize.width, windowSize.height, 0, 0, windowSize.width,
      windowSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::setupHdrOutput(int width, int height) {
  hdrOutput.fbo = {};
  hdrOutput.tex = {};
  bloomBrightTex = {};
  hdrOutput.tex.storage(1, GL_RGBA32F, {width, height});
  bloomBrightTex.storage(1, GL_RGBA32F, {width, height});
  hdrOutput.fbo.attachTexture(GL_COLOR_ATTACHMENT0, hdrOutput.tex);
  hdrOutput.fbo.attachTexture(GL_COLOR_ATTACHMENT1, bloomBrightTex);
  constexpr GLenum attachments[2] = {GL_COLOR_ATTACHMENT0,
                                     GL_COLOR_ATTACHMENT1};
  glNamedFramebufferDrawBuffers(hdrOutput.fbo.id(), 2, attachments);
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
  lightFbo.diffuse.storage(1, GL_RGB32F, {width, height});
  lightFbo.specular = {};
  lightFbo.specular.storage(1, GL_RGB32F, {width, height});
  lightFbo.fbo = {};
  lightFbo.fbo.attachTexture(GL_COLOR_ATTACHMENT0, lightFbo.diffuse);
  lightFbo.fbo.attachTexture(GL_COLOR_ATTACHMENT1, lightFbo.specular);

  constexpr GLenum attachments[2] = {GL_COLOR_ATTACHMENT0,
                                     GL_COLOR_ATTACHMENT1};
  glNamedFramebufferDrawBuffers(lightFbo.fbo.id(), 2, attachments);
}
