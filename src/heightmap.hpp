#pragma once

#include <engine/image.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>
#include <string>
#include <string_view>

class Heightmap : public engine::scene::Node {
  Heightmap(gl::Texture&& tex, gl::Program&& prog)
      : texture(std::move(tex)), program(std::move(prog)),
        engine::scene::Node(false, true) {}

public:
  static std::expected<Heightmap, std::string> fromFile(std::string_view file);

  void render(const engine::Camera& camera) override;

protected:
  glm::vec3 scale = {5000, 500, 5000};
  gl::Texture texture;
  gl::Program program;
};