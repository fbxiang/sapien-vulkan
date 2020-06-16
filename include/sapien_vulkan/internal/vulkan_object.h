#pragma once
#include "vulkan_material.h"
#include "vulkan.h"

namespace svulkan
{

struct VulkanObject {
  vk::Device mDevice;
  std::shared_ptr<VulkanMesh> mMesh = nullptr;
  std::shared_ptr<VulkanMaterial> mMaterial = nullptr;

  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;

  VulkanObject(vk::PhysicalDevice physicalDevice, vk::Device device, vk::DescriptorPool descriptorPool,
               vk::DescriptorSetLayout descriptorLayout);

  void setMesh(std::shared_ptr<VulkanMesh> mesh);

  void setMaterial(std::shared_ptr<VulkanMaterial> material);
};

}
