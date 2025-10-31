#pragma once

#include <engine/mesh.hpp>
#include <engine/mesh_animation.hpp>
#include <engine/mesh_material.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>

class Goober : public engine::scene::Node {
  Goober(engine::Mesh&& mesh, gl::Program&& program,
         engine::mesh::Animation&& animation, engine::mesh::Material&& material,
         std::vector<gl::Texture>&& textures);

public:
  static std::expected<Goober, std::string> create();

  void update(const engine::FrameInfo& info) override;
  void render(const engine::FrameInfo& info,
              const engine::Camera& camera) override;

protected:
  engine::Mesh mesh;
  gl::Program program;
  engine::mesh::Animation animation;
  engine::mesh::Material material;
  std::vector<gl::Texture> textures;

  int currentFrame = 0;
  float frameTime = 0.0f;
};