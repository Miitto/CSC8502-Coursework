#pragma once

#include <glm/glm.hpp>

struct LightUniform {
  glm::mat4 shadowMatrix[6];
  glm::vec3 position;
  float radius;
};