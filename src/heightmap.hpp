#pragma once

#include <engine/image.hpp>
#include <engine/scene_node.hpp>
#include <expected>
#include <gl/gl.hpp>
#include <string>
#include <string_view>

class Heightmap : public engine::scene::Node {
  Heightmap(gl::Texture&& heightTex, gl::Texture&& diffuseTex,
            gl::Texture&& normalTex, gl::Program&& prog,
            gl::Program&& depthProg, gl::Program&& depthCubeProg)
      : heightTex(std::move(heightTex)), diffuseTex(std::move(diffuseTex)),
        normalTex(std::move(normalTex)), program(std::move(prog)),
        depthProgram(std::move(depthProg)),
        depthCubeProgram(std::move(depthCubeProg)),
        engine::scene::Node(engine::scene::Node::RenderType::LIT, true) {
    std::vector<gl::RawTextureHandle> handles = {this->heightTex.rawHandle(),
                                                 this->diffuseTex.rawHandle()};
    SetBoundingRadius(1250.0f);
  }

public:
  static std::expected<Heightmap, std::string>
  fromFile(std::string_view heightFile, std::string_view diffuseFile,
           std::string_view normalFile);

  void render(const engine::Frustum& frustum) override;
  void renderDepthOnly(const engine::Frustum& frustum) override;
  void renderDepthOnlyCube() override;

protected:
  gl::Texture heightTex;
  gl::Texture diffuseTex;
  gl::Texture normalTex;

  gl::Program program;
  gl::Program depthProgram;
  gl::Program depthCubeProgram;
};