#pragma once
#include "common/glm_common.h"
#include "light.h"

namespace svulkan {
struct CameraUBO {
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
  glm::mat4 viewMatrixInverse;
  glm::mat4 projectionMatrixInverse;
  glm::mat4 userData;
};

struct PBRMaterialUBO {
  glm::vec4 baseColor{0.3, 0.3, 0.3, 1};
  float specular{0};
  float roughness{1};
  float metallic{0};
  float additionalTransparency{0};
  int hasColorMap{0};
  int hasSpecularMap{0};
  int hasNormalMap{0};
  int hasHeightMap{0};
};

struct ObjectUBO {
  glm::mat4 modelMatrix;
  glm::uvec4 segmentation;
  glm::mat4 userData;
};

constexpr int NumDirectionalLights = 3;
constexpr int NumPointLights = 10;
struct SceneUBO {
  glm::vec4 ambientLight;
  DirectionalLight directionalLights[NumDirectionalLights];
  PointLight pointLights[NumPointLights];
};

} // namespace svulkan
