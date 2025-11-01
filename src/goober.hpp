#pragma once

#include <engine/mesh/animated_mesh.hpp>
#include <engine/mesh/mesh_animation.hpp>
#include <engine/mesh/mesh_material.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>

class Goober : public engine::scene::Node {
  struct TexEntry {
    gl::Texture texture;
    GLuint64 handle;
  };

  Goober(gl::Buffer&& meshBuffer, engine::AnimatedMesh&& mesh,
         gl::Program&& program, engine::mesh::Animation&& animation,
         engine::mesh::Material&& material, std::vector<TexEntry>&& textures,
         gl::Buffer&& texHandleBuffer);

public:
  static std::expected<Goober, std::string> create();

  void update(const engine::FrameInfo& info) override;
  void render(const engine::FrameInfo& info,
              const engine::Camera& camera) override;

protected:
  gl::Buffer meshBuffer;
  engine::AnimatedMesh mesh;
  gl::Program program;
  engine::mesh::Animation animation;
  engine::mesh::Material material;

  std::vector<TexEntry> textures = {};
  gl::Buffer texHandleBuffer = {};

  int currentFrame = 0;
  float frameTime = 0.0f;
};