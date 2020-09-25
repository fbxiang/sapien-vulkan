#pragma once
#include <string>

namespace svulkan {

struct VulkanRendererConfig {
  std::string shaderDir{};
  std::string culling{"back"};
  uint32_t customTextureCount{0};
};

}
