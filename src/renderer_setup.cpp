#include "renderer.hpp"

#include "heightmap.hpp"
#include "logger/logger.hpp"
#include "skybox.hpp"
#include "water.hpp"
#include <engine/image.hpp>
#include <engine/mesh/mesh.hpp>
#include <engine/mesh/mesh_material.hpp>
#include <engine/mesh_node.hpp>
#include <random>

namespace {
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

  struct EnvPath {
    std::string_view path;
    bool flip = false;

    EnvPath(std::string_view p, bool f) : path(p), flip(f) {}
    EnvPath(std::string_view p) : path(p) {}
    EnvPath(const char* p, bool f) : path(p), flip(f) {}
    EnvPath(const char* p) : path(p) {}
  };

  std::expected<gl::CubeMap, std::string>
  getEnvMap(EnvPath up, std::string_view down, std::string_view east,
            std::string_view west, std::string_view north,
            std::string_view south) {
    auto topRes = engine::Image::fromFile(up.path, up.flip, 4);
    if (!topRes) {
      return std::unexpected(
          fmt::format("Failed to load env map top: {}", topRes.error()));
    }
    auto bottomRes = engine::Image::fromFile(down, false, 4);
    if (!bottomRes) {
      return std::unexpected(
          fmt::format("Failed to load env map bottom: {}", bottomRes.error()));
    }

    auto leftRes = engine::Image::fromFile(east, false, 4);
    if (!leftRes) {
      return std::unexpected(
          fmt::format("Failed to load env map left: {}", leftRes.error()));
    }

    auto rightRes = engine::Image::fromFile(west, false, 4);
    if (!rightRes) {
      return std::unexpected(
          fmt::format("Failed to load env map right: {}", rightRes.error()));
    }
    auto frontRes = engine::Image::fromFile(north, false, 4);
    if (!frontRes) {
      return std::unexpected(
          fmt::format("Failed to load env map front: {}", frontRes.error()));
    }
    auto backRes = engine::Image::fromFile(south, false, 4);
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

} // namespace

void Renderer::setupCameraTrack() {
  track.addKeyframe(0.f, {2000.f, 1000.f, 2000.f}, {-35.f, 45.f});
  track.addKeyframe(5.f, {2000.f, 1000.f, 2000.f}, {-35.f, 45.f});
  track.addKeyframe(10.f, {100.f, 300.f, 40.f}, {-35.f, 45.f});
  track.addEffect(10.f, [&](float) { enableBloom = true; });
  track.addEffect(15.f, [&](float) { enableBloom = false; });
  track.addKeyframe(15.f, {40.f, 300.f, 45.f}, {-35.f, 0.f});
  track.addEffect(17.f, [&](float) { postProcesses[2]->disable(); });
  track.addEffect(19.f, [&](float) { postProcesses[2]->enable(); });
  track.addKeyframe(20.f, {40.f, 300.f, 45.f}, {-35.f, 0.f});
  track.addKeyframe(25.f, {-20.f, 310.f, 20.f}, {-40.f, -50.f});
  track.addKeyframe(30.f, {-1000.f, 300.f, 1000.f}, {-45.f, 225.f});
  track.addEffect(32.5f, [&](float) { postProcesses[1]->disable(); });
  track.addEffect(35.f, [&](float) { postProcesses[1]->enable(); });
  track.addKeyframe(35.f, {-1000.f, 300.f, 1000.f}, {-45.f, 225.f});
  track.addKeyframe(37.5f, {-1000.f, 300.f, 1000.f}, {35.f, 225.f});
  track.addEffect(38.f, [&](float) { postProcesses[0]->disable(); });
  track.addEffect(40.f, [&](float) { postProcesses[0]->enable(); });
  track.addKeyframe(40.f, {-1000.f, 300.f, 1000.f}, {35.f, 225.f});

  track.addKeyframe(45.f, {1360.f, 300.f, 1360.f}, {-35.f, 45});
  track.addEffect(47.5, [&](float) { enableDebugUi = true; });

  track.addEffect(50.f, 50.5f, [&](float t) {
    float factor = t / 0.5f;
    float cubicT = factor < 0.5f ? 4.f * factor * factor * factor
                                 : 1.f - powf(-2.f * factor + 2.f, 3) / 2.f;

    camera.setSplitRatio(cubicT * 0.5f);
  });
  track.addEffect(50.55f, [&](float) { camera.setSplitRatio(0.5f); });
  track.addKeyframe(55.f, {1360.f, 300.f, 1360.f}, {-35.f, 45});
  track.addEffect(55.f, [&](float) { enableDebugUi = false; });
  track.addEffect(55.f, 55.5f, [&](float t) {
    float factor = t / 0.5f;
    float cubicT = factor < 0.5f ? 4.f * factor * factor * factor
                                 : 1.f - powf(-2.f * factor + 2.f, 3) / 2.f;

    camera.setSplitRatio(cubicT * 0.5f + 0.5f);
  });
  track.addKeyframe(60.f, {300.f, 350.f, 300.f}, {-35.f, 45.f});
  track.addKeyframe(65.f, {300.f, 350.f, 300.f}, {-35.f, 45.f});
  track.addKeyframe(70.f, {-300, 350.f, 300.f}, {-35.f, 315.f});
  track.addKeyframe(75.f, {-300, 350.f, -300.f}, {-35.f, 225.f});
  track.addKeyframe(80.f, {300.f, 350.f, -300.f}, {-35.f, 135.f});
  track.addEffect(80.f, [&](float) {
    onTrack = false;
    enableDebugUi = true;
  });
  track.addKeyframe(81.f, {300.f, 350.f, -300.f}, {-35.f, 45.f});
}

bool Renderer::setupMeshes() {
  {
    auto pointLightMeshDataOpt =
        engine::mesh::Data::fromFile(MESHDIR "Sphere.msh");
    if (!pointLightMeshDataOpt) {
      Logger::error("Failed to load point light mesh: {}",
                    pointLightMeshDataOpt.error());
      return true;
    }
    pointLightMesh = engine::mesh::BasicMesh(pointLightMeshDataOpt.value());
  }

  {
    auto spotLightMeshDataOpt =
        engine::mesh::Data::fromFile(MESHDIR "Cone.msh");
    if (!spotLightMeshDataOpt) {
      Logger::error("Failed to load spot light mesh: {}",
                    spotLightMeshDataOpt.error());
      return true;
    }
    spotLightMesh = engine::mesh::BasicMesh(spotLightMeshDataOpt.value());
  }

  auto heightmapResult = Heightmap::fromFile(TEXTUREDIR "terrain/height.png",
                                             TEXTUREDIR "terrain/diffuse.png",
                                             TEXTUREDIR "terrain/normal.png");
  if (!heightmapResult) {
    Logger::error("Failed to load heightmap: {}", heightmapResult.error());
    return true;
  }
  graph.AddChild(
      std::make_shared<Heightmap>(std::move(heightmapResult.value())));

  auto summerHeightmapResult = Heightmap::fromFile(
      TEXTUREDIR "terrain/height.png", TEXTUREDIR "terrain/diffuse_summer.png",
      TEXTUREDIR "terrain/normal.png");
  if (!summerHeightmapResult) {
    Logger::error("Failed to load summer heightmap: {}",
                  summerHeightmapResult.error());
    return true;
  }
  rightGraph.AddChild(
      std::make_shared<Heightmap>(std::move(summerHeightmapResult.value())));

  graph.AddChild(std::make_shared<Water>(5000.0f, 110.0f, envMap));
  rightGraph.AddChild(std::make_shared<Water>(5000.0f, 250.0f, envMap));

  auto gooberMeshDataOpt = engine::mesh::Data::fromFile(MESHDIR "Role_T.msh");
  if (!gooberMeshDataOpt) {
    Logger::error("Failed to load goober mesh: {}", gooberMeshDataOpt.error());
    return true;
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

      auto texSetRes = createTextureSet(*matEntry, fmt::format("Goober {}", i));
      if (!texSetRes) {
        Logger::error("Failed to create goober texture set: {}",
                      texSetRes.error());
        return true;
      }
      gooberTexs.push_back(std::move(texSetRes.value()));
    }
  }

  engine::mesh::Mesh gooberMesh(gooberMeshData, std::move(gooberTexs));

  struct MeshWithData {
    engine::mesh::Mesh& mesh;
    const engine::mesh::Data& data;
    std::optional<engine::mesh::Animation*> anim;
  };

  MeshWithData gooberWithData = {gooberMesh, gooberMeshData, &gooberAnimation};

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

  staticVertexSize = vertices * sizeof(engine::mesh::WeightedVertex);

  uint32_t indexOffset =
      gl::Buffer::roundToAlignment(staticVertexSize, sizeof(uint32_t));
  uint32_t indicesSize = indices * sizeof(uint32_t);

  jointOffset = gl::Buffer::roundToAlignment(
      indexOffset + indicesSize, gl::UNIFORM_BUFFER_OFFSET_ALIGNMENT);
  jointSize = joints * sizeof(glm::mat4);

  uint32_t bufferSize = jointOffset + jointSize;

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

    gl::MappingRef sm = {stagingMapping, jointOffset};
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

  std::mt19937 rng(12345);
  std::uniform_int_distribution<uint32_t> gooberAnimPos(
      0, gooberAnimation.GetFrameCount());

  constexpr std::array<glm::vec3, 5> gooberSetups = {{
      {9.5f, 268.75f, 0.0f},
      {25.f, 268.75f, 0.0f},
      {40.f, 268.75f, 0.0f},
      {55.f, 268.75f, 0.0f},
      {70.f, 268.75f, 0.0f},
  }};

  auto gooberMeshPtr =
      std::make_shared<engine::mesh::Mesh>(std::move(gooberMesh));

  for (auto& position : gooberSetups) {
    std::shared_ptr gooberNode =
        std::make_shared<engine::scene::MeshNode>(gooberMeshPtr);
    gooberNode->SetTransform(glm::translate(glm::mat4(1.0f), position));
    gooberNode->SetScale(glm::vec3(10.f));
    gooberNode->SetBoundingRadius(15.f);
    gooberNode->setFrame(gooberAnimPos(rng));
    graph.AddChild(std::move(gooberNode));
  }

  for (float x = 1000.f; x <= 1300.f; x += 30.f) {
    for (float z = 1000.f; z <= 1300.f; z += 30.f) {
      glm::vec3 position = {x, 110.f, z};
      std::shared_ptr gooberNode =
          std::make_shared<engine::scene::MeshNode>(gooberMeshPtr);
      gooberNode->SetTransform(glm::translate(glm::mat4(1.0f), position));
      gooberNode->SetScale(glm::vec3(10.f));
      gooberNode->SetBoundingRadius(15.f);
      gooberNode->setFrame(gooberAnimPos(rng));
      graph.AddChild(std::move(gooberNode));
    }
  }

  batchVao.attribFormat(0, 3, GL_FLOAT, GL_FALSE,
                        offsetof(engine::mesh::Vertex, position), 0);
  batchVao.attribFormat(1, 2, GL_FLOAT, GL_FALSE,
                        offsetof(engine::mesh::Vertex, texCoord), 0);
  batchVao.attribFormat(2, 3, GL_FLOAT, GL_FALSE,
                        offsetof(engine::mesh::Vertex, normal), 0);
  batchVao.attribFormat(3, 4, GL_FLOAT, GL_FALSE,
                        offsetof(engine::mesh::Vertex, tangent), 0);
  batchVao.attribFormat(4, 4, GL_FLOAT, GL_FALSE, 0, 1);
  batchVao.attribFormat(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 1);
  batchVao.attribFormat(6, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), 1);
  batchVao.attribFormat(7, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec4), 1);
  batchVao.bufferDivisor(1, 1);

  return false;
}

bool Renderer::setupPostProcesses() {
  auto copyPPOpt = PostProcess::create("Copy", SHADERDIR "tex.frag.glsl");
  if (!copyPPOpt) {
    Logger::error("Failed to create test post process: {}", copyPPOpt.error());
    bail();
    return true;
  }
  copyPP = std::move(*copyPPOpt);

  auto blurPPOpt = Blur::create();
  if (!blurPPOpt) {
    Logger::error("Failed to create blur post process: {}", blurPPOpt.error());
    bail();
    return true;
  }
  blurPP = std::move(*blurPPOpt);

  auto bloomPPOpt =
      PostProcess::create("Bloom", SHADERDIR "postprocess/bloom.frag.glsl");
  if (!bloomPPOpt) {
    Logger::error("Failed to create bloom post process: {}",
                  bloomPPOpt.error());
    bail();
    return true;
  }
  bloomPP = std::move(*bloomPPOpt);

  auto skyboxRes = Skybox::create(envMap);
  if (!skyboxRes) {
    Logger::error("Failed to create skybox: {}", skyboxRes.error());
    bail();
    return true;
  }

  std::unique_ptr<PostProcess> skyboxPtr =
      std::make_unique<Skybox>(std::move(*skyboxRes));

  postProcesses.emplace_back(std::move(skyboxPtr));

  auto reflectionsOpt = PostProcess::create(
      "Reflections", SHADERDIR "postprocess/reflections.frag.glsl");
  if (!reflectionsOpt) {
    Logger::error("Failed to create reflections post process");
    bail();
    return true;
  }
  std::unique_ptr<PostProcess> reflectionsPPtr =
      std::make_unique<PostProcess>(std::move(*reflectionsOpt));

  postProcesses.emplace_back(std::move(reflectionsPPtr));

  auto fxaaRes =
      PostProcess::create("FXAA", SHADERDIR "postprocess/fxaa.frag.glsl");
  if (!fxaaRes) {
    Logger::error("Failed to create FXAA post process: {}", fxaaRes.error());
    bail();
    return true;
  }

  std::unique_ptr<PostProcess> fxaaPtr =
      std::make_unique<PostProcess>(std::move(*fxaaRes));
  postProcesses.emplace_back(std::move(fxaaPtr));

  return false;
}

bool Renderer::setupShaders() {
  auto skinProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "compute/skin.comp.glsl", gl::Shader::Type::COMPUTE}});
  if (!skinProgramOpt) {
    Logger::error("Failed to create skinning program: {}",
                  skinProgramOpt.error());
    bail();
    return true;
  }
  skinProgram = std::move(*skinProgramOpt);

  auto batchProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "batch.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "tex_bindless.frag.glsl", gl::Shader::Type::FRAGMENT}});
  if (!batchProgramOpt) {
    Logger::error("Failed to create batch program: {}",
                  batchProgramOpt.error());
    bail();
    return true;
  }
  batchProgram = std::move(*batchProgramOpt);

  auto batchShadowProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "batch_shadow.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/depth_to_linear.frag.glsl",
        gl::Shader::Type::FRAGMENT}});
  if (!batchShadowProgramOpt) {
    Logger::error("Failed to create batch shadow program: {}",
                  batchShadowProgramOpt.error());
    bail();
    return true;
  }
  batchShadowProgram = std::move(*batchShadowProgramOpt);

  auto batchShadowCubeProgramOpt = gl::Program::fromFiles(
      {{SHADERDIR "batch_shadow_cube.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/omni_shadow.geom.glsl", gl::Shader::Type::GEOMETRY},
       {SHADERDIR "lighting/depth_to_linear.frag.glsl",
        gl::Shader::Type::FRAGMENT}});
  if (!batchShadowCubeProgramOpt) {
    Logger::error("Failed to create batch shadow cube program: {}",
                  batchShadowCubeProgramOpt.error());
    bail();
    return true;
  }
  batchShadowCubeProgram = std::move(*batchShadowCubeProgramOpt);

  auto pointLightOpt = gl::Program::fromFiles(
      {{SHADERDIR "lighting/point_light.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/point_light.frag.glsl",
        gl::Shader::Type::FRAGMENT}});
  if (!pointLightOpt) {
    Logger::error("Failed to create deferred light program: {}",
                  pointLightOpt.error());
    bail();
    return true;
  }
  pointLight = std::move(*pointLightOpt);

  auto spotLightOpt = gl::Program::fromFiles(
      {{SHADERDIR "lighting/spot_light.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/spot_light.frag.glsl",
        gl::Shader::Type::FRAGMENT}});
  if (!spotLightOpt) {
    Logger::error("Failed to create spot light program: {}",
                  spotLightOpt.error());
    bail();
    return true;
  }
  spotLight = std::move(*spotLightOpt);

  auto deferredLightCombineOpt = gl::Program::fromFiles(
      {{SHADERDIR "fullscreen.vert.glsl", gl::Shader::Type::VERTEX},
       {SHADERDIR "lighting/combine.frag.glsl", gl::Shader::Type::FRAGMENT}});

  if (!deferredLightCombineOpt) {
    Logger::error("Failed to create deferred light combine program: {}",
                  deferredLightCombineOpt.error());
    bail();
    return true;
  }
  deferredLightCombine = std::move(*deferredLightCombineOpt);

  return false;
}

void Renderer::setupLights() {
  pointLights.emplace_back(glm::vec3(100, 300, -50),
                           glm::vec4(0.2, 0.2, 0.8, 50.0), 100.f);
  pointLights.emplace_back(glm::vec3(0, 300, 30),
                           glm::vec4(0.8, 0.2, 0.2, 50.0), 100.f);
  pointLights.emplace_back(glm::vec3(2000, 1000, 2000),
                           glm::vec4(0.8, 0.8, 0.8, 20.0), 5000.f);

  rightPointLights.emplace_back(glm::vec3(0, 1500, -2500),
                                glm::vec4(0.5, 0.5, 0.5, 0.5), 5000.f);

  rightPointLights.emplace_back(glm::vec3(25, 300, 30),
                                glm::vec4(0.2, 0.8, 0.2, 50.0), 100.f);
  rightPointLights.emplace_back(glm::vec3(150, 300, -50),
                                glm::vec4(0.8, 0.8, 0.2, 50.0), 100.f);
  rightPointLights.emplace_back(glm::vec3(300, 300, 100),
                                glm::vec4(0.8, 0.2, 0.8, 50.0), 100.f);
  rightPointLights.emplace_back(glm::vec3(400, 300, -150),
                                glm::vec4(0.2, 0.8, 0.8, 50.0), 100.f);

  rightSpotLights.emplace_back(glm::vec3(0, 300, -50), glm::vec3(1, 0, 0),
                               glm::vec4(0.8, 0.5, 0.5, 50), 500.f);

  shadowMatrixBuffers.resize(pointLights.size() + rightPointLights.size());
  for (auto& buf : shadowMatrixBuffers) {
    buf.buffer.init(sizeof(PointLight::LightUniform), nullptr,
                    gl::Buffer::Usage::WRITE | gl::Buffer::Usage::PERSISTENT |
                        gl::Buffer::Usage::COHERENT);
    buf.mapping = buf.buffer.map(gl::Buffer::Mapping::WRITE |
                                 gl::Buffer::Mapping::PERSISTENT |
                                 gl::Buffer::Mapping::COHERENT);
  }
  spotShadowMatrixBuffers.resize(spotLights.size() + rightSpotLights.size());
  for (auto& buf : spotShadowMatrixBuffers) {
    buf.buffer.init(sizeof(SpotLight::LightUniform), nullptr,
                    gl::Buffer::Usage::WRITE | gl::Buffer::Usage::PERSISTENT |
                        gl::Buffer::Usage::COHERENT);
    buf.mapping = buf.buffer.map(gl::Buffer::Mapping::WRITE |
                                 gl::Buffer::Mapping::PERSISTENT |
                                 gl::Buffer::Mapping::COHERENT);
  }
}

Renderer::Renderer(int width, int height, const char title[])
    : engine::App(width, height, title, true),
      camera(
          engine::PerspectiveCamera{0.1f, 10000.0f,
                                    static_cast<float>(windowSize.width) /
                                        static_cast<float>(windowSize.height),
                                    glm::radians(90.0f)},
          engine::PerspectiveCamera{0.1f, 10000.0f,
                                    static_cast<float>(windowSize.width) /
                                        static_cast<float>(windowSize.height),
                                    glm::radians(90.f)},
          {windowSize.width, windowSize.height}) {
  camera.setSplitRatio(0.0f);
  camera.onResize(windowSize.width, windowSize.height);

  camera.left().SetPosition({0.f, 300.f, 0.0f});
  camera.left().SetRotation(glm::quat(glm::radians(glm::vec3(-35, 270, 0))));
  camera.left().EnableMouse(false);

  camera.right().SetPosition({300, 350, 300});
  camera.right().SetRotation(glm::quat(glm::radians(glm::vec3(-35, 45, 0))));
  camera.right().EnableMouse(false);

  setupCameraTrack();

  auto cubeMapResult = getEnvMap(TEXTUREDIR "envmaps/rusted_up.jpg",
                                 TEXTUREDIR "envmaps/rusted_down.jpg",
                                 TEXTUREDIR "envmaps/rusted_east.jpg",
                                 TEXTUREDIR "envmaps/rusted_west.jpg",
                                 TEXTUREDIR "envmaps/rusted_north.jpg",
                                 TEXTUREDIR "envmaps/rusted_south.jpg");
  if (!cubeMapResult) {
    Logger::error("Failed to load environment map: {}", cubeMapResult.error());
    bail();
    return;
  }
  envMap = std::move(*cubeMapResult);

  auto nightCubeMapResult =
      getEnvMap({TEXTUREDIR "envmaps/AllSkyFree/ColdNightUp.png", true},
                TEXTUREDIR "envmaps/AllSkyFree/ColdNightDown.png",
                TEXTUREDIR "envmaps/AllSkyFree/ColdNightLeft.png",
                TEXTUREDIR "envmaps/AllSkyFree/ColdNightRight.png",
                TEXTUREDIR "envmaps/AllSkyFree/ColdNightFront.png",
                TEXTUREDIR "envmaps/AllSkyFree/ColdNightBack.png");
  if (!nightCubeMapResult) {
    Logger::error("Failed to load night environment map: {}",
                  nightCubeMapResult.error());
    bail();
    return;
  }
  nightEnvMap = std::move(*nightCubeMapResult);

  if (setupShaders()) {
    bail();
    return;
  }
  if (setupPostProcesses()) {
    bail();
    return;
  }

  if (setupMeshes()) {
    bail();
    return;
  }

  setupLights();

  batchVao.bindIndexBuffer(staticBuffer.id());
  batchVao.label("Batch Vao");

  setupHdrOutput(windowSize.width, windowSize.height);
  setupPostProcesses(windowSize.width, windowSize.height);
  setupLightFbo(windowSize.width, windowSize.height);
}
