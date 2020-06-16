#pragma once
#include "vulkan.h"
#include "sapien_vulkan/uniform_buffers.h"

namespace svulkan
{
class VulkanScene {
  std::unique_ptr<VulkanBufferData> mUBO = nullptr;
  vk::UniqueDescriptorSet mDescriptorSet {};

  vk::Device mDevice;

 public:
  VulkanScene(vk::PhysicalDevice physicalDevice, vk::Device device, vk::DescriptorPool descriptorPool,
              vk::DescriptorSetLayout descriptorLayout);
  void updateUBO(SceneUBO const&ubo);
  inline vk::DescriptorSet getDescriptorSet() const { return mDescriptorSet.get(); }
};
}
