#pragma once

#include "blur.hpp"
#include "cameraTrack.hpp"
#include "pointLight.hpp"
#include "postprocess.hpp"
#include <array>
#include <engine/app.hpp>
#include <engine/mesh/basic.hpp>
#include <engine/split_camera.hpp>
#include <gl/gl.hpp>
#include <memory>
#include <spotLight.hpp>

class Renderer : public engine::App {
public:
  Renderer(int width, int height, const char title[]);

  bool update(const engine::FrameInfo& frame) override;
  void render(const engine::FrameInfo& frame) override;

  void onWindowResize(engine::Window::Size newSize) override;

private:
  void setupCameraTrack();
  bool setupMeshes();
  bool setupPostProcesses();
  bool setupShaders();
  void setupLights();

  void debugUi(const engine::FrameInfo& frame);

  struct BatchSetup {
    uint32_t textureOffset;
    uint32_t textureSize;
    uint32_t draws;
  };

  BatchSetup setupBatches();
  void renderLit(const engine::scene::Graph::NodeLists& nodeLists,
                 const BatchSetup& batch, const engine::Camera& camera,
                 GLuint offset);
  void renderPointLights();
  void renderSpotLights();
  bool combineDeferredLightBuffers();
  void renderPostProcesses();

  engine::SplitCamera<engine::PerspectiveCamera, engine::PerspectiveCamera>
      camera;

  void useLeftCamera();
  void useRightCamera();

  engine::scene::Graph graph;
  engine::scene::Graph rightGraph;

  bool onTrack = true;
  CameraTrack track = {};

  GLuint staticVertexSize = 0;
  GLuint jointOffset = 0;
  GLuint jointSize = 0;
  GLuint rightIndirectOffset = 0;

  gl::Program skinProgram;

  gl::Buffer staticBuffer;
  gl::Buffer skinnedVerticesBuffer;
  gl::Buffer dynamicBuffer;
  gl::Mapping dynamicMapping;

  gl::Vao batchVao;
  gl::Program batchProgram;

  gl::Program batchShadowProgram;
  gl::Program batchShadowCubeProgram;

  struct MappedBuffer {
    gl::Buffer buffer;
    gl::Mapping mapping;
  };

  std::vector<MappedBuffer> shadowMatrixBuffers;
  std::vector<MappedBuffer> spotShadowMatrixBuffers;

  gl::Program pointLight;
  gl::Program spotLight;
  gl::Program deferredLightCombine;

  struct LightFbo {
    gl::Texture diffuse = {};
    gl::Texture specular = {};
    gl::Framebuffer fbo = {};
  };

  LightFbo lightFbo = {};

  engine::mesh::BasicMesh pointLightMesh;
  std::vector<PointLight> pointLights = {};
  std::vector<PointLight> rightPointLights = {};

  engine::mesh::BasicMesh spotLightMesh;
  std::vector<SpotLight> spotLights = {};
  std::vector<SpotLight> rightSpotLights = {};

  gl::CubeMap envMap = {};
  gl::CubeMap nightEnvMap = {};

  std::vector<std::unique_ptr<PostProcess>> postProcesses;
  PostProcess copyPP;
  Blur blurPP;
  PostProcess bloomPP;
  bool enableBloom = false;

  bool enableDebugUi = false;

  struct Fbos {
    gl::Texture tex = {};
    gl::Framebuffer fbo = {};
  };

  Fbos hdrOutput = {};
  gl::Texture bloomBrightTex = {};

  std::array<Fbos, 2> postProcessFlipFlops = {Fbos{}, Fbos{}};

  void setupHdrOutput(int width, int height);
  void setupPostProcesses(int width, int height);
  void setupLightFbo(int width, int height);
};