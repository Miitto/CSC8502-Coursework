#pragma once

#include <array>
#include <engine/camera.hpp>
#include <engine/frustum.hpp>
#include <gl/gl.hpp>
#include <glm/glm.hpp>
#include <glm\ext\matrix_clip_space.hpp>
#include <glm\ext\matrix_transform.hpp>

class SpotLight {
public:
  constexpr static int32_t SHADOW_MAP_SIZE = 4096;

  struct LightUniform {
    glm::mat4 shadowMatrix;
    glm::vec3 position;
    float radius;
    glm::vec3 direction;
    float padding = 0.0f;
  };

  struct InstanceData {
    glm::vec3 position = glm::vec3(0.0);
    float radius = 1.0;
    glm::vec4 color = glm::vec4(1.0);
    glm::vec3 direction = glm::vec3(0.0, -1.0, 0.0);
    float padding = 0.0f;
  };

  SpotLight(const glm::vec3& position, const glm::vec3& direction,
            const glm::vec4& color, float radius)
      : m({position, radius, color, glm::normalize(direction)}) {
    setupShadowMap();
  }

  void writeInstanceData(const gl::MappingRef mapping) const {
    mapping.write(&m, sizeof(InstanceData), 0);
  }

  constexpr static GLuint dataSize() {
    return static_cast<GLuint>(sizeof(InstanceData));
  }

  const glm::vec3& position() const { return m.position; }
  const glm::vec4& color() const { return m.color; }
  const float& radius() const { return m.radius; }
  const glm::vec3& direction() const { return m.direction; }
  glm::mat4 modelMatrix() const {
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation[2] = glm::vec4(m.direction, 0.0f);
    rotation[0] = glm::vec4(
        glm::normalize(glm::cross(glm::vec3(0.0, 1.0, 0.0), m.direction)),
        0.0f);
    rotation[1] = glm::vec4(
        glm::cross(m.direction,
                   glm::vec3(rotation[0].x, rotation[0].y, rotation[0].z)),
        0.0f);

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), m.position);
    glm::mat4 scale =
        glm::scale(glm::mat4(1.0f), glm::vec3(m.radius, m.radius, m.radius));

    return translation * rotation * scale;
  }

  void setupShadowMap() {
    shadowMap.storage(1, GL_DEPTH_COMPONENT24,
                      gl::Texture::Size{SHADOW_MAP_SIZE, SHADOW_MAP_SIZE});
    shadowMap.setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    shadowMap.setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    shadowMapHandle = shadowMap.createHandle();

    shadowFbo.attachTexture(GL_DEPTH_ATTACHMENT, shadowMap.id(), 0);
    glNamedFramebufferDrawBuffer(shadowFbo.id(), GL_NONE);
    glNamedFramebufferReadBuffer(shadowFbo.id(), GL_NONE);
  }

  void renderShadowMap(
      std::function<void(const engine::Frustum&, const glm::vec3&)> renderFn,
      const gl::Mapping& matrixMapping) const {

    glm::mat4 perspective =
        glm::perspective(glm::radians(90.0f), 1.0f, m.radius, .1f);

    shadowFbo.bind();
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    LightUniform uniformData = {};
    uniformData.position = m.position;
    uniformData.radius = m.radius;

    glm::mat4 shadowView =
        glm::lookAt(m.position, m.position + m.direction,
                    abs(m.direction.y) == 1.0 ? glm::vec3(0.0, 0.0, -1.0)
                                              : glm::vec3(0.0, -1.0, 0.0));

    glm::mat4 shadowViewProj = perspective * shadowView;
    uniformData.shadowMatrix = shadowViewProj;

    matrixMapping.write(&uniformData, sizeof(LightUniform), 0);

    engine::Frustum shadowFrustum(shadowViewProj);

    renderFn(shadowFrustum, m.position);
    glMemoryBarrier(GL_UNIFORM_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT |
                    GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
  }

  const gl::Texture& getShadowMap() const { return shadowMap; }
  const gl::TextureHandle& getShadowMapHandle() const {
    return shadowMapHandle;
  }

protected:
  InstanceData m;

  gl::TextureHandle shadowMapHandle = 0;
  gl::Texture shadowMap;

  gl::Framebuffer shadowFbo = {};
};