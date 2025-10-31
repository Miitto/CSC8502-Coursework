#include "Robot.h"
#include "logger/logger.hpp"
#include <engine\camera.hpp>
#include <engine\constants.hpp>
#include <glm\ext\matrix_transform.hpp>

using engine::scene::Node;

Robot::Robot(std::shared_ptr<engine::Mesh>& cube)
    : engine::scene::Node(false, true), program(gl::Program()),
      modelMatsBuffer({6 * sizeof(glm::mat4), nullptr,
                       gl::Buffer::Usage::DYNAMIC | gl::Buffer::Usage::WRITE |
                           gl::Buffer::Usage::PERSISTENT |
                           gl::Buffer::Usage::COHERENT}) {

  auto basicProgram_opt = gl::Program::fromFiles(
      {{SHADERDIR "basic.vert.glsl", gl::Shader::VERTEX},
       {SHADERDIR "basic.frag.glsl", gl::Shader::FRAGMENT}});
  if (!basicProgram_opt.has_value()) {
    Logger::critical("Failed to link basic shader program!");
    return;
  }
  program = std::move(basicProgram_opt.value());

  modelMatsBuffer.map(gl::Buffer::Mapping::WRITE |
                      gl::Buffer::Mapping::PERSISTENT |
                      gl::Buffer::Mapping::COHERENT);

  body = std::make_shared<MeshNode>(cube, glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),
                                    false, false);
  body->SetScale({10, 15, 5});
  body->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3(0, 35, 0)));
  body->SetBoundingRadius(15.0f);

  auto head = std::make_shared<MeshNode>(
      cube, glm::vec4(0.2f, 0.8f, 0.2f, 1.0f), false, false);
  head->SetScale({5, 5, 5});
  head->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{0, 30, 0}));
  head->SetBoundingRadius(5.0f);

  auto leftArm = std::make_shared<MeshNode>(
      cube, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f), false, false);
  leftArm->SetScale({3, -18, 3});
  leftArm->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{-12, 30, -1}));
  leftArm->SetBoundingRadius(18.0f);

  auto rightArm = std::make_shared<MeshNode>(
      cube, glm::vec4(0.2f, 0.2f, 0.8f, 1.0f), false, false);
  rightArm->SetScale({3, -18, 3});
  rightArm->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{12, 30, -1}));
  rightArm->SetBoundingRadius(18.0f);

  auto leftLeg = std::make_shared<MeshNode>(
      cube, glm::vec4(0.8f, 0.8f, 0.2f, 1.0f), false, false);
  leftLeg->SetScale({3, -17.5, 3});
  leftLeg->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{-8, 0, 0}));
  leftLeg->SetBoundingRadius(17.5f);

  auto rightLeg = std::make_shared<MeshNode>(
      cube, glm::vec4(0.8f, 0.8f, 0.2f, 1.0f), false, false);
  rightLeg->SetScale({3, -17.5, 3});
  rightLeg->SetTransform(glm::translate(glm::mat4(1.0), glm::vec3{8, 0, 0}));
  rightLeg->SetBoundingRadius(17.5f);

  AddChild(body);
  body->AddChild(head);
  body->AddChild(leftArm);
  body->AddChild(rightArm);
  body->AddChild(leftLeg);
  body->AddChild(rightLeg);

  SetBoundingRadius(50.f);

  this->head = head.get();
  this->leftArm = leftArm.get();
  this->rightArm = rightArm.get();
  this->leftLeg = leftLeg.get();
  this->rightLeg = rightLeg.get();
}

void Robot::update(const engine::FrameInfo& info) {
  const float dt = info.frameDelta;
  m_transforms.local =
      m_transforms.local *
      glm::rotate(glm::mat4(1.0), glm::radians(30.f * dt), engine::UP);

  head->SetTransform(
      head->GetTransforms().local *
      glm::rotate(glm::mat4(1.0), glm::radians(-30.f * dt), engine::UP));

  leftArm->SetTransform(
      leftArm->GetTransforms().local *
      glm::rotate(glm::mat4(1.0), glm::radians(-30.f * dt), engine::RIGHT));
  rightArm->SetTransform(
      rightArm->GetTransforms().local *
      glm::rotate(glm::mat4(1.0), glm::radians(30.f * dt), engine::RIGHT));

  Node::update(info);
}

void Robot::render(const engine::FrameInfo& info,
                   const engine::Camera& camera) {
  (void)info;
  program.bind();
  writeModelMatrices();
  camera.bindMatrixBuffer(0);
  modelMatsBuffer.bindBase(gl::StorageBuffer::Target::STORAGE, 1);

  std::array<MeshNode*, 6> meshNodes = {body.get(), head,    leftArm,
                                        rightArm,   leftLeg, rightLeg};

  for (size_t i = 0; i < meshNodes.size(); ++i) {
    glUniform1i(0, static_cast<GLuint>(i));
    meshNodes[i]->draw();
  }
}

void Robot::writeModelMatrices() {
  auto& mapping = modelMatsBuffer.getMapping();
  std::array<MeshNode*, 6> meshNodes = {body.get(), head,    leftArm,
                                        rightArm,   leftLeg, rightLeg};

  for (GLuint i = 0; i < meshNodes.size(); ++i) {
    glm::mat4 modelMatrix = glm::scale(meshNodes[i]->GetTransforms().world,
                                       meshNodes[i]->GetScale());
    mapping.write(&modelMatrix, sizeof(glm::mat4), i * sizeof(glm::mat4));
  }
}
