#pragma once

#include <engine/image.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>
#include <string>
#include <string_view>

class Heightmap : public engine::scene::Node {
  Heightmap(gl::Texture&& heightTex, gl::Texture&& diffuseTex,
            gl::Program&& prog)
      : heightTex(std::move(heightTex)), diffuseTex(std::move(diffuseTex)),
        program(std::move(prog)), engine::scene::Node(false, true) {
    std::vector<gl::RawTextureHandle> handles = {this->heightTex.rawHandle(),
                                                 this->diffuseTex.rawHandle()};
    SetBoundingRadius(1250.0f);
    textureBuffer.init(sizeof(gl::RawTextureHandle) * 2, handles.data());
  }

public:
  static std::expected<Heightmap, std::string>
  fromFile(std::string_view heightFile, std::string_view diffuseFile);

  void render(const engine::FrameInfo& info, const engine::Camera& camera,
              const engine::Frustum& frustum) override;

protected:
  // Don't need a VAO since we have no vertex attribs, but OpenGL requires one
  // anyway :/
  gl::Vao dummyVao = {};

  gl::Texture heightTex;
  gl::Texture diffuseTex;

  gl::Program program;
  gl::Buffer textureBuffer = {};
};