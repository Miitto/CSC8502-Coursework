#pragma once

#include <engine/mesh.hpp>
#include <engine/scene_node.hpp>
#include <gl/shaders.hpp>

class MeshNode : public engine::scene::Node {
  std::shared_ptr<engine::Mesh> mesh;
  glm::vec4 color;

public:
  MeshNode(std::shared_ptr<engine::Mesh>& mesh, const glm::vec4& materialColor,
           bool transparent, bool render = true)
      : engine::scene::Node(transparent, render), mesh(mesh),
        color(materialColor) {}

  void draw() {
    if (mesh) {
      glUniform4f(1, color.x, color.y, color.z, color.w);
      mesh->Draw();
    }
  }

  void render(const engine::FrameInfo& info,
              const engine::Camera& camera) override {
    draw();
    Node::render(info, camera);
  }
};

class Robot : public engine::scene::Node {

public:
  Robot() = delete;
  Robot(std::shared_ptr<engine::Mesh>& cube);
  virtual ~Robot() = default;

  void update(const engine::FrameInfo& info) override;
  virtual void render(const engine::FrameInfo& info,
                      const engine::Camera& camera) override;
  inline bool isValid() const { return program.isValid(); }

protected:
  void writeModelMatrices();

  gl::Program program;
  gl::StorageBuffer modelMatsBuffer;

  std::shared_ptr<MeshNode> body;
  MeshNode* head;
  MeshNode* leftArm;
  MeshNode* rightArm;
  MeshNode* leftLeg;
  MeshNode* rightLeg;
};
