#pragma once
#include <string>

namespace svulkan {

struct VulkanRendererConfig {
  std::string shaderDir{};
  std::string culling{"back"};
  uint32_t customTextureCount{0};
  bool useShadowMap{false};
  uint32_t shadowMapSize{2048};
};

} // namespace svulkan
