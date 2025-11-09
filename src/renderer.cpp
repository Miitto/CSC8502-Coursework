#include "renderer.hpp"

#include "heightmap.hpp"
#include "light.hpp"
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
    DEPTH,
    DIFFUSE_LIGHT,
    SPECULAR_LIGHT
  };

  DebugView debugView = DebugView::NONE;

  std::expected<engine::mesh::TextureSet, std::string>
  createTextureSet(const engine::mesh::MaterialEntry& matEntry,
                   const std::string_view name) {
    engine::mesh::TextureSet texSet;
    auto diffuseImgPathOpt = matEntry.GetEntry("Diffuse");
    if (!diffuseImgPathOpt) {
      return std::unexpected(
          fmt::format("Material {} missing diffuse texture", name));
    }
    auto diffuseRes = engine::Image::fromFile(
        std::string(TEXTUREDIR) + diffuseImgPathOpt->data(), true, 4);
    if (!diffuseRes) {
      return std::unexpected(
          fmt::format("Failed to load diffuse texture: {} for {}",
                      diffuseRes.error(), name));
    }
    auto diffuseTex = diffuseRes->toTexture(-1);
    diffuseTex.label(fmt::format("{} Diffuse", name).c_str());
    diffuseTex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    diffuseTex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    texSet.images.diffuse = std::move(diffuseTex);
    auto diffuseHandle = texSet.images.diffuse.createHandle();
    if (!diffuseHandle.isValid()) {
      return std::unexpected(
          fmt::format("Failed to create diffuse texture handle for {}", name));
    }
    diffuseHandle.use();
    texSet.handles.diffuse = diffuseHandle.handle();

    auto normalImgPathOpt = matEntry.GetEntry("Normal");
    if (normalImgPathOpt) {
      auto normalRes = engine::Image::fromFile(
          std::string(TEXTUREDIR) + (normalImgPathOpt->data()), true, 4);
      if (!normalRes) {
        return std::unexpected(
            fmt::format("Failed to load goober normal texture: {} for {}",
                        normalRes.error(), name));
      }
      auto normalTex = normalRes->toTexture(-1);
      normalTex.label(fmt::format("{} Normal", name).c_str());
      normalTex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      normalTex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      texSet.images.bump = std::move(normalTex);
      auto normalHandle = texSet.images.bump->createHandle();
      if (!normalHandle.isValid()) {
        return std::unexpected(fmt::format(
            "Failed to create goober normal texture handle for {}", name));
      }
      normalHandle.use();
      texSet.handles.bump = normalHandle.handle();
    }

    auto materialImgPathOpt = matEntry.GetEntry("Material");
    if (materialImgPathOpt) {
      auto materialRes = engine::Image::fromFile(
          std::string(TEXTUREDIR) + (materialImgPathOpt->data()), true, 4);
      if (!materialRes) {
        return std::unexpected(
            fmt::format("Failed to load goober material texture: {} for {}",
                        materialRes.error(), name));
      }
      auto materialTex = materialRes->toTexture(-1);
      materialTex.label(fmt::format("{} Material", name).c_str());
      materialTex.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      materialTex.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      texSet.images.material = std::move(materialTex);
      auto materialHandle = texSet.images.material->createHandle();
      if (!materialHandle.isValid()) {
        return std::unexpected(fmt::format(
            "Failed to create goober material texture handle for {}", name));
      }
      materialHandle.use();
      texSet.handles.material = materialHandle.handle();
    }

    return texSet;
  }

  std::optional<std::string> setupMeshes(engine::scene::Graph& graph,
                                         gl::Vao& vao, GLuint& verticesSize,
                                         GLuint& jointsOffset,
                                         GLuint& jointsSize,
                                         gl::Buffer& staticBuffer) {

    auto heightmapResult = Heightmap::fromFile(TEXTUREDIR "terrain/height.png",
                                               TEXTUREDIR "terrain/diffuse.png",
                                               TEXTUREDIR "terrain/normal.png");
    if (!heightmapResult) {
      return fmt::format("Failed to load heightmap: {}",
                         heightmapResult.error());
    }
    graph.AddChild(
        std::make_shared<Heightmap>(std::move(heightmapResult.value())));

    auto gooberMeshDataOpt = engine::mesh::Data::fromFile(MESHDIR "Role_T.msh");
    if (!gooberMeshDataOpt) {
      return gooberMeshDataOpt.error();
    }
    auto& gooberMeshData = gooberMeshDataOpt.value();
    engine::mesh::Animation gooberAnimation(MESHDIR "Role_T.anm");

    engine::mesh::Material gooberMat(MESHDIR "Role_T.mat");

    std::vector<engine::mesh::TextureSet> gooberTexs;

    {
      for (size_t i = 0; i < gooberMeshData.meshLayers().size(); i++) {
        auto matEntry = gooberMat.GetMaterialForLayer(static_cast<int>(i));
        if (!matEntry) {
          continue;
        }

        auto texSetRes =
            createTextureSet(*matEntry, fmt::format("Goober {}", i));
        if (!texSetRes) {
          return texSetRes.error();
        }
        gooberTexs.push_back(std::move(texSetRes.value()));
      }
    }

    engine::mesh::Mesh gooberMesh(gooberMeshData, std::move(gooberTexs));

    struct MeshWithData {
      engine::mesh::Mesh& mesh;
      engine::mesh::Data& data;
      std::optional<engine::mesh::Animation*> anim;
    };

    MeshWithData gooberWithData = {gooberMesh, gooberMeshData,
                                   &gooberAnimation};

    std::vector<MeshWithData> meshes = {gooberWithData};

    uint32_t vertices = 0;
    uint32_t indices = 0;
    uint32_t joints = 0;
    for (const auto& mesh : meshes) {
      vertices += static_cast<uint32_t>(mesh.data.vertices().size());
      indices += static_cast<uint32_t>(mesh.data.indices().size());
      if (mesh.anim)
        joints += (**mesh.anim).GetFrameCount() * (**mesh.anim).GetJointCount();
    }

    verticesSize = vertices * sizeof(engine::mesh::WeightedVertex);

    uint32_t indexOffset =
        gl::Buffer::roundToAlignment(verticesSize, sizeof(uint32_t));
    uint32_t indicesSize = indices * sizeof(uint32_t);

    jointsOffset = gl::Buffer::roundToAlignment(
        indexOffset + indicesSize, gl::UNIFORM_BUFFER_OFFSET_ALIGNMENT);
    jointsSize = joints * sizeof(glm::mat4);

    uint32_t bufferSize = jointsOffset + jointsSize;

    gl::Buffer stagingBuffer(bufferSize, nullptr,
                             gl::Buffer::Usage::WRITE |
                                 gl::Buffer::Usage::DYNAMIC);
    {
      auto stagingMapping = stagingBuffer.map(gl::Buffer::Mapping::WRITE);
      {
        gl::MappingRef sm = stagingMapping;

        GLuint written = 0;
        for (const auto& mesh : meshes) {
          mesh.mesh.writeVertexData(gooberMeshData, written, sm);

          sm += sizeof(engine::mesh::WeightedVertex);
        }
      }

      {
        gl::MappingRef sm = {stagingMapping, indexOffset};
        GLuint offset = indexOffset;
        for (const auto& mesh : meshes) {
          mesh.mesh.writeIndexData(mesh.data, offset, sm);
          sm += static_cast<uint32_t>(mesh.data.indices().size() *
                                      sizeof(uint32_t));
        }
      }

      gl::MappingRef sm = {stagingMapping, jointsOffset};
      uint32_t jointsWritten = 0;
      for (const auto& mesh : meshes) {
        if (mesh.anim) {
          auto anim = *mesh.anim.value();
          mesh.mesh.writeJointData(mesh.data, anim, sm, jointsWritten);
          auto writtenJoints = anim.GetFrameCount() * anim.GetJointCount();
          sm += writtenJoints * sizeof(glm::mat4);
          jointsWritten += writtenJoints;
        }
      }
    }

    staticBuffer.init(bufferSize);
    staticBuffer.label("Static Mesh Buffer");
    stagingBuffer.copyTo(staticBuffer, 0, 0, bufferSize);

    struct GooberSetup {
      glm::vec3 position;
      uint32_t frame;
    };

    constexpr std::array<GooberSetup, 5> gooberSetups = {{
        {{9.5f, 268.75f, 0.0f}, 0},
        {{25.f, 268.75f, 0.0f}, 5},
        {{40.f, 268.75f, 0.0f}, 10},
        {{55.f, 268.75f, 0.0f}, 15},
        {{70.f, 268.75f, 0.0f}, 20},
    }};

    auto gooberMeshPtr =
        std::make_shared<engine::mesh::Mesh>(std::move(gooberMesh));

    for (auto& setup : gooberSetups) {
      std::shared_ptr gooberNode =
          std::make_shared<engine::scene::MeshNode>(gooberMeshPtr);
      gooberNode->SetTransform(glm::translate(glm::mat4(1.0f), setup.position));
      gooberNode->SetScale(glm::vec3(10.f));
      gooberNode->SetBoundingRadius(15.f);
      gooberNode->setFrame(setup.frame);
      graph.AddChild(std::move(gooberNode));
    }

    vao.attribFormat(0, 3, GL_FLOAT, GL_FALSE,
                     offsetof(engine::mesh::Vertex, position), 0);
    vao.attribFormat(1, 2, GL_FLOAT, GL_FALSE,
                     offsetof(engine::mesh::Vertex, texCoord), 0);
    vao.attribFormat(2, 3, GL_FLOAT, GL_FALSE,
                     offsetof(engine::mesh::Vertex, normal), 0);
    vao.attribFormat(3, 4, GL_FLOAT, GL_FALSE,
                     offsetof(engine::mesh::Vertex, tangent), 0);
    vao.attribFormat(4, 4, GL_FLOAT, GL_FALSE, 0, 1);
    vao.attribFormat(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 1);
    vao.attribFormat(6, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), 1);
    vao.attribFormat(7, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec4), 1);
    vao.bufferDivisor(1, 1);

    return std::nullopt;
  }

  std::expected<gl::CubeMap, std::string> getEnvMap() {
    auto topRes =
        engine::Image::fromFile(TEXTUREDIR "envmaps/rusted_up.jpg", false, 4);
    if (!topRes) {
      return std::unexpected(
          fmt::format("Failed to load env map top: {}", topRes.error()));
    }
    auto bottomRes =
        engine::Image::fromFile(TEXTUREDIR "envmaps/rusted_down.jpg", false, 4);
    if (!bottomRes) {
      return std::unexpected(
          fmt::format("Failed to load env map bottom: {}", bottomRes.error()));
    }

    auto leftRes =
        engine::Image::fromFile(TEXTUREDIR "envmaps/rusted_east.jpg", false, 4);
    if (!leftRes) {
      return std::unexpected(
          fmt::format("Failed to load env map left: {}", leftRes.error()));
    }

    auto rightRes =
        engine::Image::fromFile(TEXTUREDIR "envmaps/rusted_west.jpg", false, 4);
    if (!rightRes) {
      return std::unexpected(
          fmt::format("Failed to load env map right: {}", rightRes.error()));
    }
    auto frontRes = engine::Image::fromFile(
        TEXTUREDIR "envmaps/rusted_north.jpg", false, 4);
    if (!frontRes) {
      return std::unexpected(
          fmt::format("Failed to load env map front: {}", frontRes.error()));
    }
    auto backRes = engine::Image::fromFile(
        TEXTUREDIR "envmaps/rusted_south.jpg", false, 4);
    if (!backRes) {
      return std::unexpected(
          fmt::format("Failed to load env map back: {}", backRes.error()));
    }

    gl::CubeMap cubeMap;
    glm::ivec2 size = topRes->getDimensions();

    if (leftRes->getDimensions() != size || rightRes->getDimensions() != size ||
        frontRes->getDimensions() != size || backRes->getDimensions() != size ||
        bottomRes->getDimensions() != size) {
      return std::unexpected("Env map faces have mismatched dimensions");
    }

    auto mipLevels = gl::Texture::calcMipLevels(size.x, size.y);

    cubeMap.storage(mipLevels + 1, GL_RGBA8, size);
    cubeMap.subImage(0, 0, 0, gl::CubeMap::Face::POSITIVE_Y, size.x, size.y, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, topRes->getData());
    cubeMap.subImage(0, 0, 0, gl::CubeMap::Face::NEGATIVE_Y, size.x, size.y, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, bottomRes->getData());
    cubeMap.subImage(0, 0, 0, gl::CubeMap::Face::NEGATIVE_X, size.x, size.y, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, leftRes->getData());
    cubeMap.subImage(0, 0, 0, gl::CubeMap::Face::POSITIVE_X, size.x, size.y, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, rightRes->getData());
    cubeMap.subImage(0, 0, 0, gl::CubeMap::Face::NEGATIVE_Z, size.x, size.y, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, frontRes->getData());
    cubeMap.subImage(0, 0, 0, gl::CubeMap::Face::POSITIVE_Z, size.x, size.y, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, backRes->getData());
    cubeMap.generateMipmaps();

    cubeMap.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    cubeMap.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    cubeMap.setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    cubeMap.setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    cubeMap.setParameter(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return cubeMap;
  }

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
      camera(0.1f, 10000.0f,
             static_cast<float>(width) / static_cast<float>(height),
             glm::radians(90.0f)) {

  camera.onResize(width, height);

  camera.SetPosition({0.f, 300.f, 0.0f});

  auto cubeMapResult = getEnvMap();
  if (!cubeMapResult) {
    Logger::error("Failed to load environment map: {}", cubeMapResult.error());
    bail();
    return;
  }
  envMap = std::move(*cubeMapResult);

  auto copyPPOpt = PostProcess::create("Copy", SHADERDIR "tex.frag.glsl");
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

  auto meshSetupError = setupMeshes(graph, batchVao, staticVertexSize,
                                    jointOffset, jointSize, staticBuffer);
  if (meshSetupError) {
    Logger::error("Failed to setup meshes: {}", *meshSetupError);
    bail();
    return;
  }
  batchVao.bindIndexBuffer(staticBuffer.id());
  batchVao.label("Batch Vao");

  auto skinProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "compute/skin.comp.glsl", gl::Shader::Type::COMPUTE}});
  if (!skinProgramOpt) {
    Logger::error("Failed to create skinning program: {}",
                  skinProgramOpt.error());
    bail();
    return;
  }
  skinProgram = std::move(*skinProgramOpt);

  auto batchProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "batch.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "tex_bindless.frag.glsl", gl::Shader::Type::FRAGMENT}});
  if (!batchProgramOpt) {
    Logger::error("Failed to create batch program: {}",
                  batchProgramOpt.error());
    bail();
    return;
  }
  batchProgram = std::move(*batchProgramOpt);

  auto batchShadowProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "batch_shadow.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/shadow.geom.glsl", gl::Shader::Type::GEOMETRY},
       {SHADERDIR "lighting/depth_to_linear.frag.glsl",
        gl::Shader::Type::FRAGMENT}});
  if (!batchShadowProgramOpt) {
    Logger::error("Failed to create batch shadow program: {}",
                  batchShadowProgramOpt.error());
    bail();
    return;
  }
  batchShadowProgram = std::move(*batchShadowProgramOpt);

  shadowMatrixBuffer.init(sizeof(LightUniform), nullptr,
                          gl::Buffer::Usage::WRITE |
                              gl::Buffer::Usage::PERSISTENT |
                              gl::Buffer::Usage::COHERENT);
  shadowMatrixMapping = shadowMatrixBuffer.map(gl::Buffer::Mapping::WRITE |
                                               gl::Buffer::Mapping::PERSISTENT |
                                               gl::Buffer::Mapping::COHERENT);

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

  /*pointLights.emplace_back(glm::vec3(0, 300, 0), glm::vec4(0.8, 0.8,
     0.8, 2.0), 500.f);*/
  pointLights.emplace_back(glm::vec3(100, 300, 100),
                           glm::vec4(0.8, 0.2, 0.2, 20.0), 300.f);

  auto instanceSize =
      static_cast<GLuint>(pointLights.size()) * PointLight::dataSize();

  pointLightBuffer.init(instanceSize, nullptr,
                        gl::Buffer::Usage::DYNAMIC | gl::Buffer::Usage::WRITE |
                            gl::Buffer::Usage::PERSISTENT |
                            gl::Buffer::Usage::COHERENT);
  pointLightMapping = pointLightBuffer.map(gl::Buffer::Mapping::WRITE |
                                           gl::Buffer::Mapping::PERSISTENT |
                                           gl::Buffer::Mapping::COHERENT);

  graph.AddChild(std::make_shared<Water>(5000.0f, 110.0f, envMap));

  auto skyboxRes = Skybox::create(envMap);
  if (!skyboxRes) {
    Logger::error("Failed to create skybox: {}", skyboxRes.error());
    bail();
    return;
  }

  std::unique_ptr<PostProcess> skyboxPtr =
      std::make_unique<Skybox>(std::move(*skyboxRes));

  postProcesses.emplace_back(std::move(skyboxPtr));

  auto reflectionsOpt = PostProcess::create(
      "Reflections", SHADERDIR "postprocess/reflections.frag.glsl");
  if (!reflectionsOpt) {
    Logger::error("Failed to create reflections post process");
    bail();
    return;
  }
  std::unique_ptr<PostProcess> reflectionsPPtr =
      std::make_unique<PostProcess>(std::move(*reflectionsOpt));

  postProcesses.emplace_back(std::move(reflectionsPPtr));

  auto fxaaRes =
      PostProcess::create("FXAA", SHADERDIR "postprocess/fxaa.frag.glsl");
  if (!fxaaRes) {
    Logger::error("Failed to create FXAA post process: {}", fxaaRes.error());
    bail();
    return;
  }

  std::unique_ptr<PostProcess> fxaaPtr =
      std::make_unique<PostProcess>(std::move(*fxaaRes));
  postProcesses.emplace_back(std::move(fxaaPtr));

  setupPostProcesses(width, height);
  setupLightFbo(width, height);
}

void Renderer::onWindowResize(engine::Window::Size newSize) {
  App::onWindowResize(newSize);
  camera.onResize(newSize.width, newSize.height);
  setupPostProcesses(newSize.width, newSize.height);
  setupLightFbo(newSize.width, newSize.height);
}

void Renderer::update(const engine::FrameInfo& info) {
  engine::App::update(info);
  camera.update(input, info.frameDelta);

  graph.update(info);
}

void Renderer::render(const engine::FrameInfo& info) {
  (void)info;
  gbuffers->fbo.bind();
  glViewport(0, 0, windowSize.width, windowSize.height);
  glScissor(0, 0, windowSize.width, windowSize.height);
  glClearDepth(0.0f);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto nodeLists =
      graph.BuildNodeLists(camera.GetFrustum(), camera.GetPosition());

  debugUi(info, nodeLists);

  auto batch = setupBatches();
  renderLit(nodeLists, batch);

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
    case DebugView::DEPTH: {
      auto bg = engine::globals::DUMMY_VAO.bindGuard();
      depthView.bind();
      glUniform1f(1, camera.getNear());
      glUniform1f(0, camera.getFar());
      gbuffers->depthStencil.bind(0);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      return;
    }
    default:
      break;
    }
    auto bg = engine::globals::DUMMY_VAO.bindGuard();
    copyPP.run([]() {});
    return;
  }

  if (!renderPointLights())
    return;
  auto bg = engine::globals::DUMMY_VAO.bindGuard();
  if (!combineDeferredLightBuffers())
    return;
  renderPostProcesses();
  renderLightGizmos();
}

Renderer::BatchSetup Renderer::setupBatches() {
  auto& roots = graph.GetRoots();

  engine::scene::Node::DrawParams drawParams = {0, 0, 0};
  for (const auto& node : roots) {
    drawParams += node->getBatchDrawParams();
  }

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
    for (const auto& node : roots) {
      node->skinVertices(writtenVertices);
    }
  }

  auto indirectSize = static_cast<GLuint>(
      drawParams.maxIndirectCmds * sizeof(gl::DrawElementsIndirectCommand));
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
  for (auto& node : roots) {
    node->writeInstanceData(instanceMap, writtenInstances, textureMap);
  }

  return {
      .textureOffset = textureOffset,
      .textureSize = textureSize,
  };
}

void Renderer::renderLit(const engine::scene::Graph::NodeLists& nodeLists,
                         const BatchSetup& batch) {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_GREATER);
  glClear(GL_DEPTH_BUFFER_BIT);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  glDisable(GL_BLEND);

  camera.bindMatrixBuffer(0);

  // Will likely be the larger more custom stuff like terrain
  nodeLists.renderLit(camera.GetFrustum());

  auto bg = batchVao.bindGuard();
  batchProgram.bind();
  gl::MappingRef indirectMap = {dynamicMapping, 0};
  auto draws = writeLitDraws(nodeLists, indirectMap);
  dynamicBuffer.bind(gl::Buffer::BasicTarget::DRAW_INDIRECT);

  dynamicBuffer.bindRange(gl::Buffer::StorageTarget::STORAGE, 2,
                          batch.textureOffset, batch.textureSize);

  glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, draws,
                              sizeof(gl::DrawElementsIndirectCommand));
}
void Renderer::debugUi(const engine::FrameInfo& frame,
                       const engine::scene::Graph::NodeLists& nodeLists) {
  (void)frame;

  engine::gui::GuiWindow cameraFrame("Camera");
  camera.CameraDebugUI();
  ImGui::Text("L: %d | O: %d | T: %d", nodeLists.lit.size(),
              nodeLists.opaque.size(), nodeLists.transparent.size());
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

  ImGui::SeparatorText("Post Processes");
  for (auto& pp : postProcesses) {
    bool enabled = pp->isEnabled();
    if (ImGui::Checkbox(pp->name().data(), &enabled)) {
      pp->setEnabled(enabled);
    }
  }
}

bool Renderer::renderPointLights() {
  {
    glViewport(0, 0, PointLight::SHADOW_MAP_SIZE, PointLight::SHADOW_MAP_SIZE);
    glScissor(0, 0, PointLight::SHADOW_MAP_SIZE, PointLight::SHADOW_MAP_SIZE);
    auto bg = batchVao.bindGuard();

    GLuint writtenDraws = 0;
    gl::MappingRef indirectMap = {dynamicMapping, 0};
    for (const auto& root : graph.GetRoots()) {
      root->writeBatchedDraws(indirectMap, writtenDraws);
    }

    dynamicBuffer.bind(gl::Buffer::BasicTarget::DRAW_INDIRECT);
    shadowMatrixBuffer.bindBase(gl::Buffer::StorageTarget::UNIFORM, 5);

    glCullFace(GL_FRONT);

    auto renderFn = [&]() {
      for (const auto& root : graph.GetRoots()) {
        root->renderDepthOnly();
      }

      batchVao.bind();
      batchShadowProgram.bind();

      glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr,
                                  writtenDraws,
                                  sizeof(gl::DrawElementsIndirectCommand));
    };

    for (const auto& light : pointLights) {
      light.renderShadowMap(renderFn, shadowMatrixMapping);
    }

    gl::Vao::unbind();
  }
  camera.bindMatrixBuffer(0);
  glViewport(0, 0, windowSize.width, windowSize.height);
  glScissor(0, 0, windowSize.width, windowSize.height);
  glCullFace(GL_BACK);

  pointLight.bind();

  auto bg = engine::globals::DUMMY_VAO.bindGuard();
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

  for (size_t i = 0; i < pointLights.size(); ++i) {
    glUniform3fv(0, 1, &pointLights[i].position()[0]);
    glUniform1f(1, pointLights[i].radius());
    glUniform4fv(2, 1, &pointLights[i].color()[0]);
    pointLights[i].getShadowMap().bind(4);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }

  if (debugView == DebugView::DIFFUSE_LIGHT ||
      debugView == DebugView::SPECULAR_LIGHT) {
    gl::Framebuffer::unbind();
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

    return false;
  }

  return true;
}

bool Renderer::combineDeferredLightBuffers() {
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  static glm::vec3 ambientLight(0.1f, 0.1f, 0.1f);

  {
    auto frame = engine::gui::GuiWindow("Lighting");
    ImGui::ColorEdit3("Ambient Light", &ambientLight.x);
  }

  deferredLightCombine.bind();
  postProcessFlipFlops[0].fbo.bind();
  gbuffers->diffuse.bind(0);
  lightFbo.diffuse.bind(1);
  lightFbo.specular.bind(2);

  glUniform3fv(0, 1, &ambientLight.x);

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

  gbuffers->normal.bind(1);
  gbuffers->material.bind(2);
  gbuffers->depthStencil.bind(3);
  envMap.bind(4);
  lightFbo.diffuse.bind(5);
  lightFbo.specular.bind(6);

  for (size_t i = 0; i < postProcesses.size(); ++i) {
    auto& pp = postProcesses[i];
    if (!pp->isEnabled())
      continue;
    flip();
    pp->run(flip);
  }
  gl::Framebuffer::unbind();
  postProcessFlipFlops[bound].fbo.blit(
      0, 0, 0, windowSize.width, windowSize.height, 0, 0, windowSize.width,
      windowSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer::renderLightGizmos() {
  auto bg = engine::globals::DUMMY_VAO.bindGuard();
  pointLight.bind();
  glUniform1ui(3, 1);
  for (size_t i = 0; i < pointLights.size(); ++i) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniform3fv(0, 1, &pointLights[i].position()[0]);
    glUniform1f(1, pointLights[i].radius());
    glUniform4fv(2, 1, &pointLights[i].color()[0]);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
  lightFbo.diffuse.storage(1, GL_RGBA32F, {width, height});
  lightFbo.specular = {};
  lightFbo.specular.storage(1, GL_RGBA32F, {width, height});
  lightFbo.fbo = {};
  lightFbo.fbo.attachTexture(GL_COLOR_ATTACHMENT0, lightFbo.diffuse);
  lightFbo.fbo.attachTexture(GL_COLOR_ATTACHMENT1, lightFbo.specular);

  constexpr GLenum attachments[2] = {GL_COLOR_ATTACHMENT0,
                                     GL_COLOR_ATTACHMENT1};
  glNamedFramebufferDrawBuffers(lightFbo.fbo.id(), 2, attachments);
}
