#pragma once

#include "pointLight.hpp"
#include "postprocess.hpp"
#include <array>
#include <engine/app.hpp>
#include <engine/camera.hpp>
#include <gl/gl.hpp>
#include <memory>

class Renderer : public engine::App {
public:
  Renderer(int width, int height, const char title[]);

  void update(const engine::FrameInfo& frame) override;
  void render(const engine::FrameInfo& frame) override;

  void onWindowResize(engine::Window::Size newSize) override;

private:
  void debugUi(const engine::FrameInfo& frame,
               const engine::scene::Graph::NodeLists& nodeLists);

  struct BatchSetup {
    uint32_t textureOffset;
    uint32_t textureSize;
    uint32_t draws;
  };

  BatchSetup setupBatches();
  void renderLit(const engine::scene::Graph::NodeLists& nodeLists,
                 const BatchSetup& batch);
  bool renderPointLights();
  bool combineDeferredLightBuffers();
  void renderPostProcesses();
  void renderLightGizmos();

  engine::PerspectiveCamera camera;

  engine::scene::Graph graph;

  GLuint staticVertexSize = 0;
  GLuint jointOffset = 0;
  GLuint jointSize = 0;

  gl::Program skinProgram;

  gl::Buffer staticBuffer;
  gl::Buffer skinnedVerticesBuffer;
  gl::Buffer dynamicBuffer;
  gl::Mapping dynamicMapping;

  gl::Vao batchVao;
  gl::Program batchProgram;

  gl::Program batchShadowProgram;
  gl::Buffer shadowMatrixBuffer;
  gl::Mapping shadowMatrixMapping;

  gl::Program pointLight;
  gl::Program deferredLightCombine;

  struct LightFbo {
    gl::Texture diffuse = {};
    gl::Texture specular = {};
    gl::Framebuffer fbo = {};
  };
  LightFbo lightFbo = {};
  gl::Vao pointLightVao;
  std::vector<PointLight> pointLights = {};
  gl::Buffer pointLightBuffer = {};
  gl::Mapping pointLightMapping = {};

  gl::CubeMap envMap = {};

  std::vector<std::unique_ptr<PostProcess>> postProcesses;
  PostProcess copyPP;
  gl::Program depthView;

  struct Fbos {
    gl::Texture tex = {};
    gl::Framebuffer fbo = {};
  };

  std::array<Fbos, 2> postProcessFlipFlops = {Fbos{}, Fbos{}};

  void setupPostProcesses(int width, int height);
  void setupLightFbo(int width, int height);
};