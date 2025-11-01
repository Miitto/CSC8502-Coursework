#pragma once

#include <engine/image.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>
#include <string>
#include <string_view>

class Heightmap : public engine::scene::Node {
  Heightmap(gl::Texture&& tex, gl::Program&& prog, gl::Buffer&& lodBuffer)
      : texture(std::move(tex)), program(std::move(prog)),
        lodBuffer(std::move(lodBuffer)), engine::scene::Node(false, true) {
    writeLodParams();
  }

public:
  static std::expected<Heightmap, std::string> fromFile(std::string_view file);

  void render(const engine::FrameInfo& info,
              const engine::Camera& camera) override;

protected:
  struct LodParams {
    float minDistance = 100;
    float maxDistance = 2000;
    float minTessLevel = 4;
    float maxTessLevel = 64;
  };

  void writeLodParams() {
    auto& mapping = lodBuffer.getMapping();
    mapping.write(&lodParams, sizeof(LodParams));
  }

  LodParams lodParams;
  gl::Buffer lodBuffer;
  glm::vec3 scale = {5000, 500, 5000};
  gl::Texture texture;
  gl::Program program;
};