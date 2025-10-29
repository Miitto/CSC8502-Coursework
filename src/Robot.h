#pragma once

#include <engine/mesh.hpp>
#include <engine/scene_node.hpp>

class Robot : public engine::scene::Node {

public:
  Robot() = delete;
  Robot(std::shared_ptr<engine::Mesh>& cube);
  virtual ~Robot() = default;

  void update(float dt) override;

protected:
  engine::scene::Node* head;
  engine::scene::Node* leftArm;
  engine::scene::Node* rightArm;
};
