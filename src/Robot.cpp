#include "Robot.h"
#include <engine\constants.hpp>
#include <glm\ext\matrix_transform.hpp>

using engine::scene::Node;

class MeshNode : public engine::scene::Node {
  std::shared_ptr<engine::Mesh> mesh;

public:
  MeshNode(std::shared_ptr<engine::Mesh>& mesh, const glm::vec4& materialColor)
      : engine::scene::Node(false, true), mesh(mesh) {
    auto& colors = mesh->getColors();
    colors.resize(mesh->GetVertexCount());
    for (auto& color : colors) {
      color = materialColor;
    }
  }

  void draw() override {
    if (mesh) {
      // Set material color uniform here if needed
      mesh->Draw();
    }
    Node::draw();
  }
};

Robot::Robot(std::shared_ptr<engine::Mesh>& cube)
    : engine::scene::Node(false, true) {
  std::shared_ptr<Node> body =
      std::make_shared<MeshNode>(cube, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
  body->SetScale({10, 15, 5});
  body->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3(0, 35, 0)));
  body->SetBoundingRadius(15.0f);

  std::shared_ptr<Node> head =
      std::make_shared<MeshNode>(cube, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f));
  head->SetScale({5, 5, 5});
  head->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{0, 30, 0}));
  head->SetBoundingRadius(5.0f);

  std::shared_ptr<Node> leftArm =
      std::make_shared<MeshNode>(cube, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));
  leftArm->SetScale({3, -18, 3});
  leftArm->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{-12, 30, -1}));
  leftArm->SetBoundingRadius(18.0f);

  std::shared_ptr<Node> rightArm =
      std::make_shared<MeshNode>(cube, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f));
  rightArm->SetScale({3, -18, 3});
  rightArm->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{12, 30, -1}));
  rightArm->SetBoundingRadius(18.0f);

  std::shared_ptr<Node> leftLeg =
      std::make_shared<MeshNode>(cube, glm::vec4(0.8f, 0.8f, 0.2f, 1.0f));
  leftLeg->SetScale({3, -17.5, 3});
  leftLeg->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{-8, 0, 0}));
  leftLeg->SetBoundingRadius(17.5f);

  std::shared_ptr<Node> rightLeg =
      std::make_shared<MeshNode>(cube, glm::vec4(0.8f, 0.8f, 0.2f, 1.0f));
  rightLeg->SetScale({3, -17.5, 3});
  rightLeg->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{8, 0, 0}));
  rightLeg->SetBoundingRadius(17.5f);

  AddChild(body);
  body->AddChild(head);
  body->AddChild(leftArm);
  body->AddChild(rightArm);
  body->AddChild(leftLeg);
  body->AddChild(rightLeg);

  this->head = head.get();
  this->leftArm = leftArm.get();
  this->rightArm = rightArm.get();
}

void Robot::update(float dt) {
  m_transforms.local =
      m_transforms.local * glm::rotate(glm::mat4(1.0), 30.f * dt, engine::UP);

  head->SetTransform(head->GetTransforms().local *
                     glm::rotate(glm::mat4(1.0), -30.f * dt, engine::UP));

  leftArm->SetTransform(leftArm->GetTransforms().local *
                        glm::rotate(glm::mat4(1.0), -30.f * dt, engine::RIGHT));
  rightArm->SetTransform(rightArm->GetTransforms().local *
                         glm::rotate(glm::mat4(1.0), 30.f * dt, engine::RIGHT));

  Node::update(dt);
}
