#pragma once

#include "common/glm_common.h"
#include "common/vulkan.h"
#include "uniform_buffers.h"

namespace svulkan
{

struct VulkanMaterial {
  vk::Device mDevice;

  VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                 vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout,
                 std::shared_ptr<VulkanTextureData> defaultTexture);
  void setProperties(PBRMaterialUBO const &data);
  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;
  
  std::shared_ptr<VulkanTextureData> mDiffuseMap;
  std::shared_ptr<VulkanTextureData> mSpecularMap;
  std::shared_ptr<VulkanTextureData> mNormalMap;
  std::shared_ptr<VulkanTextureData> mHeightMap;

  void updateDescriptorSets();

#if !defined(NDEBUG)
  bool mInitialized = false;
#endif
};

}
