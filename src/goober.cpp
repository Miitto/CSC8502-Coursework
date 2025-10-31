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

  std::vector<glm::mat4> frameMatrices;
  frameMatrices.reserve(mesh.GetJointCount());

  const auto& invBindPose = mesh.GetInverseBindPose();
  const auto& frameData = animation.GetJointData(currentFrame);

  for (uint32_t i = 0; i < mesh.GetJointCount(); ++i) {
    frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
  }

  auto modelMatrix = getModelMatrix();
  glUniformMatrix4fv(0, 1, GL_FALSE,
                     reinterpret_cast<const float*>(&modelMatrix));
  int j = glGetUniformLocation(program.id(), "joints");
  Logger::debug("Joints is at {}", j);
  glUniformMatrix4fv(j, static_cast<GLsizei>(frameMatrices.size()), GL_FALSE,
                     reinterpret_cast<const float*>(frameMatrices.data()));

  for (int i = 0; i < mesh.GetSubMeshCount(); ++i) {
    textures[i].bind(0);
    mesh.DrawSubMesh(i);
  }

  engine::scene::Node::render(info, camera);
}

Goober::Goober(engine::Mesh&& mesh, gl::Program&& program,
               engine::mesh::Animation&& animation,
               engine::mesh::Material&& material,
               std::vector<gl::Texture>&& textures)
    : engine::scene::Node(false, true), mesh(std::move(mesh)),
      program(std::move(program)), animation(std::move(animation)),
      material(std::move(material)), textures(std::move(textures)) {
  SetScale({50, 50, 50});
  SetTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 250.0f, 0.0f)));
  SetBoundingRadius(100.0f);
}

std::expected<Goober, std::string> Goober::create() {
  auto meshOpt = engine::Mesh::LoadFromMeshFile(MESHDIR "Role_T.msh");
  if (!meshOpt) {
    return std::unexpected(meshOpt.error());
  }
  auto& mesh = meshOpt.value();

  auto progOpt = gl::Program::fromFiles({
      {SHADERDIR "skin.vert.glsl", gl::Shader::Type::VERTEX},
      {SHADERDIR "tex.frag.glsl", gl::Shader::Type::FRAGMENT},
  });
  if (!progOpt) {
    return std::unexpected(progOpt.error());
  }
  auto& program = progOpt.value();

  engine::mesh::Animation animation(MESHDIR "Role_T.anm");
  engine::mesh::Material material(MESHDIR "Role_T.mat");

  std::vector<gl::Texture> textures;

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

    textures.push_back(std::move(texture));
  }

  return Goober{std::move(mesh), std::move(program), std::move(animation),
                std::move(material), std::move(textures)};
}