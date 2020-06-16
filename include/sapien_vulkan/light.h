#pragma once
#include "common/glm_common.h"

namespace svulkan {
struct PointLight {
  glm::vec4 position;
  glm::vec4 emission;
};

struct DirectionalLight {
  glm::vec4 direction;
  glm::vec4 emission;
};

struct ParallelogramLight {
  glm::vec3 corner;
  glm::vec3 v1, v2;
  glm::vec3 normal;
  glm::vec3 emission;
};

} // namespace Optifuser
