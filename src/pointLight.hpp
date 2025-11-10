#pragma once

#include <array>
#include <engine/camera.hpp>
#include <engine/frustum.hpp>
#include <gl/gl.hpp>
#include <glm/glm.hpp>
#include <glm\ext\matrix_clip_space.hpp>
#include <glm\ext\matrix_transform.hpp>

class PointLight {
public:
  constexpr static int32_t SHADOW_MAP_SIZE = 4096;

  struct LightUniform {
    glm::mat4 shadowMatrix[6];
    glm::vec3 position;
    float radius;
  };

  struct InstanceData {
    glm::vec3 position = glm::vec3(0.0);
    float radius = 1.0;
    glm::vec4 color = glm::vec4(1.0);
  };

  PointLight(const glm::vec3& position, const glm::vec4& color, float radius)
      : m({position, radius, color}) {
    setupShadowMap();
  }

  void writeInstanceData(const gl::MappingRef mapping) const {
    mapping.write(&m, sizeof(InstanceData), 0);
  }

  constexpr static GLuint dataSize() {
    return static_cast<GLuint>(sizeof(InstanceData));
  }

  inline static void setupVao(const gl::Vao& vao, const gl::Buffer& buffer,
                              GLuint offset) {
    vao.attribFormat(0, 3, GL_FLOAT, false, 0);
    vao.attribFormat(1, 1, GL_FLOAT, false, offsetof(InstanceData, radius));
    vao.attribFormat(2, 4, GL_FLOAT, false, offsetof(InstanceData, color));
    vao.bindVertexBuffer(0, buffer.id(), offset, sizeof(InstanceData));
  }

  const glm::vec3& position() const { return m.position; }
  const glm::vec4& color() const { return m.color; }
  const float& radius() const { return m.radius; }

  void setupShadowMap() {
    shadowMap.storage(1, GL_DEPTH_COMPONENT24,
                      gl::Texture::Size{SHADOW_MAP_SIZE, SHADOW_MAP_SIZE});
    shadowMap.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    shadowMapHandle = shadowMap.createHandle();

    shadowFbo.attachTexture(GL_DEPTH_ATTACHMENT, shadowMap.id(), 0);
    glNamedFramebufferDrawBuffer(shadowFbo.id(), GL_NONE);
    glNamedFramebufferReadBuffer(shadowFbo.id(), GL_NONE);
  }

  void renderShadowMap(std::function<void()> renderFn,
                       const gl::Mapping& matrixMapping) const {

    glm::mat4 perspective =
        glm::perspective(glm::radians(90.0f), 1.0f, m.radius, .1f);

    shadowFbo.bind();
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    constexpr std::array<glm::vec3, 6> directions = {
        glm::vec3(1.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, -1.0),
    };

    LightUniform uniformData = {};
    uniformData.position = m.position;
    uniformData.radius = m.radius;

    for (int d = 0; d < directions.size(); ++d) {
      glm::mat4 shadowView =
          glm::lookAt(m.position, m.position + directions[d],
                      d == 2 || d == 3 ? glm::vec3(0.0, 0.0, 1.0)
                                       : glm::vec3(0.0, -1.0, 0.0));

      glm::mat4 shadowViewProj = perspective * shadowView;
      uniformData.shadowMatrix[d] = shadowViewProj;
    }

    matrixMapping.write(&uniformData, sizeof(LightUniform), 0);

    renderFn();
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT |
                    GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
  }

  const gl::CubeMap& getShadowMap() const { return shadowMap; }
  const gl::TextureHandle& getShadowMapHandle() const {
    return shadowMapHandle;
  }

protected:
  InstanceData m;

  gl::TextureHandle shadowMapHandle = 0;
  gl::CubeMap shadowMap;

  gl::Framebuffer shadowFbo = {};
};