#pragma once

#include "common/glm_common.h"
#include "common/vulkan.h"
#include "uniform_buffers.h"

namespace svulkan
{

struct VulkanMaterial {
  vk::Device mDevice;

  VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                 vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout);
  void setProperties(PBRMaterialUBO const &data);
  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;
  
  VulkanTextureData *mDiffuseMap;
  VulkanTextureData *mSpecularMap;
  VulkanTextureData *mNormalMap;
};

}
