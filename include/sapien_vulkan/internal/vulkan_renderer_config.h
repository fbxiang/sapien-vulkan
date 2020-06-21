#pragma once
#include <string>

struct VulkanRendererConfig {
  std::string shaderDir {};
  std::string culling {"back"};
};
