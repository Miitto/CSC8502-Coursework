#pragma once

#include <gl/gl.hpp>
#include <glm/glm.hpp>

class PointLight {
public:
  struct InstanceData {
    glm::vec3 position = glm::vec3(0.0);
    float radius = 1.0;
    glm::vec4 color = glm::vec4(1.0);
  };
  PointLight() : m({}) {}

  PointLight(const glm::vec3& position, const glm::vec4& color, float radius)
      : m({position, radius, color}) {}

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

protected:
  InstanceData m;
};