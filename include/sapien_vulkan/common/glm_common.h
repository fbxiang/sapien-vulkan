#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Transform {
  glm::vec3 position = {0, 0, 0};
  glm::quat rotation = glm::quat(1, 0, 0, 0);
  glm::vec3 scale = {1, 1, 1};
};
