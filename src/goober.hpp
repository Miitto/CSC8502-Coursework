#pragma once

#include <engine/mesh/mesh.hpp>
#include <engine/mesh/mesh_animation.hpp>
#include <engine/mesh/mesh_material.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>

class Goober;

class GooberInstance : public engine::scene::Node {
  void setupStatics(const Goober& goober);

public:
  GooberInstance(const Goober& goober)
      : engine::scene::Node(engine::scene::Node::RenderType::LIT, false) {
    SetScale({10, 10, 10});
    SetBoundingRadius(15.0f);

    if (frameCount == 0) {
      setupStatics(goober);
    }
  }

  void update(const engine::FrameInfo& info) override;

  bool shouldRender(const engine::Frustum& frustum) const override {
    auto center = m_transforms.world[3];
    auto radius = GetBoundingRadius();
    return frustum.SphereInFrustum(center, radius);
  }

  void writeInstanceData(const gl::MappingRef mapping) const;

  void setFrame(int frame);

protected:
  static uint32_t frameCount;
  static uint32_t jointCount;
  static float frameRate;

  int currentFrame = 0;
  float frameTime = 0.0f;
};

class Goober : public engine::scene::Node {
  Goober(gl::Buffer&& vertexBuffer, gl::Buffer&& indexBuffer,
         gl::Buffer&& jointBuffer, engine::mesh::Mesh&& mesh,
         gl::Program&& program, engine::mesh::Animation&& animation,
         engine::mesh::Material&& material, std::vector<gl::Texture>&& textures,
         gl::Buffer&& texHandleBuffer, size_t gooberCount);

public:
  static std::expected<Goober, std::string> create(size_t gooberCount = 1);

  void render(const engine::FrameInfo& info, const engine::Camera& camera,
              const engine::Frustum& frustum) override;

  size_t instanceCount() const { return instances.size(); }
  GooberInstance& operator[](size_t index) { return *instances.at(index); }

protected:
  gl::Buffer vertexBuffer;
  gl::Buffer indexBuffer;
  gl::Buffer jointBuffer;
  engine::mesh::Mesh mesh;
  gl::Program program;
  engine::mesh::Animation animation;
  engine::mesh::Material material;

  std::vector<gl::Texture> textures = {};
  gl::Buffer texHandleBuffer = {};

  std::vector<std::shared_ptr<GooberInstance>> instances;
  GLuint instanceOffset = 0;
  gl::Buffer indirectBuffer = {};
  gl::Mapping indirectMapping = {};
  gl::Buffer instanceBuffer = {};
  gl::Mapping instanceMapping = {};

  friend class GooberInstance;
};
