#include "goober.hpp"
#include "logger/logger.hpp"
#include <engine/camera.hpp>
#include <engine/image.hpp>
#include <glm\ext\matrix_transform.hpp>

void Goober::update(const engine::FrameInfo& info) {
  engine::scene::Node::update(info);

  frameTime -= info.frameDelta;
  while (frameTime < 0.0f) {
    frameTime += 1.0f / animation.GetFrameRate();
    currentFrame = (currentFrame + 1) % animation.GetFrameCount();
  }
}

void Goober::render(const engine::FrameInfo& info,
                    const engine::Camera& camera) {
  program.bind();
  camera.bindMatrixBuffer(0);

  auto modelMatrix = getModelMatrix();
  glUniformMatrix4fv(0, 1, GL_FALSE,
                     reinterpret_cast<const float*>(&modelMatrix));

  uint32_t startLocation = currentFrame * animation.GetJointCount();
  glUniform1ui(1, startLocation);
  auto jointStart = mesh.getJointOffset();
  auto jointSize = mesh.getJointSize();
  meshBuffer.bindRange(gl::Buffer::StorageTarget::STORAGE, 1, jointStart,
                       jointSize);

  for (auto& tex : textures) {
    gl::Texture::makeHandleResident(tex.handle);
  }
  texHandleBuffer.bindBase(gl::Buffer::StorageTarget::STORAGE, 2);
  mesh.BatchSubmeshes();
  for (auto& tex : textures) {
    gl::Texture::makeHandleNonResident(tex.handle);
  }

  engine::scene::Node::render(info, camera);
}

Goober::Goober(gl::Buffer&& meshBuffer, engine::AnimatedMesh&& mesh,
               gl::Program&& program, engine::mesh::Animation&& animation,
               engine::mesh::Material&& material,
               std::vector<TexEntry>&& textures, gl::Buffer&& texHandleBuffer)
    : engine::scene::Node(false, true), meshBuffer(std::move(meshBuffer)),
      mesh(std::move(mesh)), program(std::move(program)),
      animation(std::move(animation)), material(std::move(material)),
      textures(std::move(textures)),
      texHandleBuffer(std::move(texHandleBuffer)) {
  SetScale({50, 50, 50});
  SetTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 250.0f, 0.0f)));
  SetBoundingRadius(100.0f);
}

std::expected<Goober, std::string> Goober::create() {
  auto meshDataOpt = engine::mesh::Data::fromFile(MESHDIR "Role_T.msh");
  if (!meshDataOpt) {
    return std::unexpected(meshDataOpt.error());
  }
  auto& meshData = meshDataOpt.value();
  engine::mesh::Animation animation(MESHDIR "Role_T.anm");

  auto meshBufferSize = engine::AnimatedMesh::requiredSize(meshData, animation);
  gl::Buffer meshBuffer(meshBufferSize, nullptr, gl::Buffer::Usage::WRITE);

  // Use a closure to ensure that mapping does not outlive the mesh.
  engine::AnimatedMesh mesh = ([&]() {
    auto mapping = meshBuffer.map(gl::Buffer::Mapping::WRITE);
    return engine::AnimatedMesh(meshData,
                                engine::Mesh::BufferLocation{
                                    .id = meshBuffer.id(),
                                    .mapping = mapping,
                                    .offset = 0,
                                },
                                animation);
  })();

  auto progOpt = gl::Program::fromFiles({
      {SHADERDIR "skin.vert.glsl", gl::Shader::Type::VERTEX},
      {SHADERDIR "tex_bindless.frag.glsl", gl::Shader::Type::FRAGMENT},
  });
  if (!progOpt) {
    return std::unexpected(progOpt.error());
  }
  auto& program = progOpt.value();

  engine::mesh::Material material(MESHDIR "Role_T.mat");

  std::vector<TexEntry> textures;

  for (int i = 0; i < mesh.GetSubMeshCount(); ++i) {
    const engine::mesh::MaterialEntry* matEntry =
        material.GetMaterialForLayer(i);

    if (!matEntry) {
      return std::unexpected("No material entry for mesh layer " +
                             std::to_string(i));
    }

    auto diffuseOpt = matEntry->GetEntry("Diffuse");
    if (!diffuseOpt) {
      return std::unexpected("No diffuse texture for mesh layer " +
                             std::to_string(i));
    }

    auto& texturePath = *diffuseOpt;
    auto imgOpt =
        engine::Image::fromFile(std::string(TEXTUREDIR) + texturePath.data());
    if (!imgOpt) {
      return std::unexpected("Failed to load image: " +
                             std::string(texturePath));
    }
    auto& img = imgOpt.value();

    auto texture = img.toTexture();
    texture.generateMipmap();
    GLuint64 handle = texture.getHandle();
    if (handle == 0) {
      return std::unexpected("Failed to get texture handle for " +
                             std::string(texturePath));
    }

    textures.emplace_back(std::move(texture), handle);
  }

  std::vector<GLuint64> textureHandles;
  textureHandles.reserve(textures.size());
  for (const auto& tex : textures) {
    textureHandles.push_back(tex.handle);
  }

  gl::Buffer texHandleBuffer(
      static_cast<GLuint>(textures.size() * sizeof(GLuint64)),
      textureHandles.data());

  return Goober{std::move(meshBuffer),     std::move(mesh),
                std::move(program),        std::move(animation),
                std::move(material),       std::move(textures),
                std::move(texHandleBuffer)};
}