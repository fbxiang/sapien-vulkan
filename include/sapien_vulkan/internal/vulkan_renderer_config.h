#pragma once
#include <string>

namespace svulkan {

struct VulkanRendererConfig {
  std::string shaderDir{};
  std::string culling{"back"};
  uint32_t customTextureCount{0};
  uint32_t customInputTextureCount{0};
};

} // namespace svulkan
