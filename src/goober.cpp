#include "goober.hpp"
#include "logger/logger.hpp"
#include <engine/camera.hpp>
#include <engine/image.hpp>
#include <glm\ext\matrix_transform.hpp>
#include <ranges>

namespace {
  struct InstanceData {
    glm::mat4 modelMatrix;
    GLuint startJoint;
    GLuint flags;
    float padding[2] = {0.0, 0.0};
  };
} // namespace

uint32_t GooberInstance::frameCount = 0;
uint32_t GooberInstance::jointCount = 0;
float GooberInstance::frameRate = 0.0f;

void GooberInstance::setupStatics(const Goober& goober) {
  frameCount = goober.animation.GetFrameCount();
  jointCount = goober.animation.GetJointCount();
  frameRate = goober.animation.GetFrameRate();
}

void GooberInstance::update(const engine::FrameInfo& info) {
  engine::scene::Node::update(info);

  frameTime -= info.frameDelta;
  while (frameTime < 0.0f) {
    frameTime += 1.0f / frameRate;
    currentFrame = (currentFrame + 1) % frameCount;
  }
}

void GooberInstance::writeInstanceData(const gl::MappingRef mapping) const {
  InstanceData data{.modelMatrix = getModelMatrix(),
                    .startJoint = currentFrame * jointCount,
                    .flags = 0};
  mapping.write(&data, sizeof(InstanceData), 0);
}

void GooberInstance::setFrame(int frame) {
  if (frame < 0 || static_cast<uint32_t>(frame) >= frameCount) {
    Logger::warn("GooberInstance::setFrame: Frame {} out of bounds (0-{})",
                 frame, frameCount - 1);
    return;
  }
  currentFrame = frame;
  frameTime = 0.0f;
}

void Goober::render(const engine::FrameInfo& info, const engine::Camera& camera,
                    const engine::Frustum& frustum) {
  program.bind();
  camera.bindMatrixBuffer(0);

  GLuint instancesToRender = 0;
  for (size_t i = 0; i < instances.size(); ++i) {
    if (!instances[i]->shouldRender(frustum)) {
      continue;
    }

    instances[i]->writeInstanceData({
        instanceMapping,
        static_cast<uint32_t>(instanceOffset +
                              (instancesToRender * sizeof(InstanceData))),
    });
    ++instancesToRender;
  }

  mesh.prepSubmeshesForBatch(indirectMapping, 0, instancesToRender, 0);

  instanceBuffer.bindRange(
      gl::Buffer::StorageTarget::STORAGE, 3, instanceOffset,
      static_cast<uint32_t>(instances.size() * sizeof(InstanceData)));

  for (auto& tex : textures) {
    tex.images.diffuse.handle().use();
    if (tex.images.bump.has_value()) {
      tex.images.bump->handle().use();
    }
    if (tex.images.material.has_value()) {
      tex.images.material->handle().use();
    }
  }

  texHandleBuffer.bindBase(gl::Buffer::StorageTarget::STORAGE, 2);
  mesh.BatchSubmeshes(indirectBuffer, 0);

  for (auto& tex : textures) {
    tex.images.diffuse.handle().unuse();
    if (tex.images.bump.has_value()) {
      tex.images.bump->handle().unuse();
    }
    if (tex.images.material.has_value()) {
      tex.images.material->handle().unuse();
    }
  }

  engine::scene::Node::render(info, camera, frustum);
}

Goober::Goober(gl::Buffer&& vertexBuffer, gl::Buffer&& indexBuffer,
               gl::Buffer&& jointBuffer, engine::mesh::Mesh&& mesh,
               gl::Program&& program, engine::mesh::Animation&& animation,
               engine::mesh::Material&& material,
               std::vector<engine::mesh::TextureSet>&& textures,
               gl::Buffer&& texHandleBuffer, size_t gooberCount)
    : engine::scene::Node(engine::scene::Node::RenderType::LIT, true),
      vertexBuffer(std::move(vertexBuffer)),
      indexBuffer(std::move(indexBuffer)), jointBuffer(std::move(jointBuffer)),
      mesh(std::move(mesh)), program(std::move(program)),
      animation(std::move(animation)), material(std::move(material)),
      textures(std::move(textures)),
      texHandleBuffer(std::move(texHandleBuffer)), instances({}) {
  instances.reserve(gooberCount);
  for (size_t i = 0; i < gooberCount; ++i) {
    instances.emplace_back(std::make_shared<GooberInstance>(*this));
  }

  for (auto& goober : instances) {
    AddChild(goober);
  }

  auto indirectSize = this->mesh.indirectBufferSize();
  indirectBuffer.init(static_cast<GLuint>(indirectSize), nullptr,
                      gl::Buffer::Usage::WRITE | gl::Buffer::Usage::DYNAMIC |
                          gl::Buffer::Usage::PERSISTENT |
                          gl::Buffer::Usage::COHERENT);
  indirectMapping = std::move(indirectBuffer.map(
      gl::Buffer::Mapping::WRITE | gl::Buffer::Mapping::PERSISTENT |
      gl::Buffer::Mapping::COHERENT));

  instanceOffset = 0;
  instanceBuffer.init(
      static_cast<GLuint>(gooberCount * sizeof(InstanceData) + instanceOffset),
      nullptr,
      gl::Buffer::Usage::WRITE | gl::Buffer::Usage::PERSISTENT |
          gl::Buffer::Usage::COHERENT);
  instanceMapping = std::move(instanceBuffer.map(
      gl::Buffer::Mapping::WRITE | gl::Buffer::Mapping::PERSISTENT |
          gl::Buffer::Mapping::COHERENT,
      0,
      static_cast<GLuint>(gooberCount * sizeof(InstanceData) +
                          instanceOffset)));

  this->mesh.bindInstanceBuffer(1, instanceBuffer, 0, sizeof(InstanceData), 1);
  this->mesh.setupInstanceAttr(7, 4, GL_FLOAT, GL_FALSE, 0, 1);
  this->mesh.setupInstanceAttr(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 1);
  this->mesh.setupInstanceAttr(9, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * 2,
                               1);
  this->mesh.setupInstanceAttr(10, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * 3,
                               1);
  this->mesh.setupInstanceAttr(11, 1, GL_UNSIGNED_INT, GL_FALSE,
                               offsetof(InstanceData, startJoint), 1);
}

std::expected<Goober, std::string> Goober::create(size_t instances) {
  auto meshDataOpt = engine::mesh::Data::fromFile(MESHDIR "Role_T.msh");
  if (!meshDataOpt) {
    return std::unexpected(meshDataOpt.error());
  }
  auto& meshData = meshDataOpt.value();
  engine::mesh::Animation animation(MESHDIR "Role_T.anm");

  engine::mesh::Mesh mesh(meshData);

  auto vertexBufferSize = engine::mesh::Mesh::vertexDataSize(meshData);
  gl::Buffer vertexBuffer(vertexBufferSize);
  vertexBuffer.label("Goober Vertex Buffer");

  auto indexBufferSize = engine::mesh::Mesh::indexDataSize(meshData, 0);
  gl::Buffer indexBuffer(indexBufferSize.size);
  indexBuffer.label("Goober Index Buffer");

  auto jointBufferSize = engine::mesh::Mesh::jointDataSize(animation, 0);
  gl::Buffer jointBuffer(jointBufferSize.size);
  jointBuffer.label("Goober Joint Buffer");

  auto stagingBufferSize = vertexBufferSize + indexBufferSize.alignedSize +
                           jointBufferSize.alignedSize;
  gl::Buffer stagingBuffer(stagingBufferSize, nullptr,
                           gl::Buffer::Usage::WRITE |
                               gl::Buffer::Usage::DYNAMIC);

  {
    auto mapping = stagingBuffer.map(gl::Buffer::Mapping::WRITE);

    mesh.writeVertexData(meshData, {vertexBuffer, 0}, {mapping, 0});
    mesh.writeIndexData(meshData, {indexBuffer, 0},
                        {mapping, vertexBufferSize});
    mesh.writeJointData(
        meshData, animation, {jointBuffer, 0}, 1,
        {mapping, vertexBufferSize + indexBufferSize.alignedSize});
  }

  stagingBuffer.copyTo(vertexBuffer, 0, 0, vertexBufferSize);
  stagingBuffer.copyTo(indexBuffer, vertexBufferSize, 0,
                       indexBufferSize.alignedSize);
  stagingBuffer.copyTo(jointBuffer,
                       vertexBufferSize + indexBufferSize.alignedSize, 0,
                       jointBufferSize.alignedSize);

  auto progOpt = gl::Program::fromFiles({
      {SHADERDIR "skin.vert.glsl", gl::Shader::Type::VERTEX},
      {SHADERDIR "tex_bindless.frag.glsl", gl::Shader::Type::FRAGMENT},
  });
  if (!progOpt) {
    return std::unexpected(progOpt.error());
  }
  auto& program = progOpt.value();

  engine::mesh::Material material(MESHDIR "Role_T.mat");

  std::vector<engine::mesh::TextureSet> textures;

  for (int i = 0; i < mesh.GetSubMeshCount(); ++i) {
    const engine::mesh::MaterialEntry* matEntry =
        material.GetMaterialForLayer(i);

    if (!matEntry) {
      return std::unexpected("No material entry for mesh layer " +
                             std::to_string(i));
    }

    auto texRes = matEntry->LoadTextures(TEXTUREDIR);
    if (!texRes) {
      return std::unexpected("Failed to load textures for mesh layer " +
                             std::to_string(i) + ": " + texRes.error());
    }
    textures.emplace_back(std::move(*texRes));
  }

  std::vector<engine::mesh::TextureHandleSet> textureHandles;
  textureHandles.reserve(textures.size());
  for (const auto& tex : textures) {
    textureHandles.push_back({tex.handles});
  }

  gl::Buffer texHandleBuffer(
      static_cast<GLuint>(textures.size() *
                          sizeof(engine::mesh::TextureHandleSet)),
      textureHandles.data());

  return Goober{std::move(vertexBuffer),    std::move(indexBuffer),
                std::move(jointBuffer),     std::move(mesh),
                std::move(program),         std::move(animation),
                std::move(material),        std::move(textures),
                std::move(texHandleBuffer), instances};
}