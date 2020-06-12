#include "vulkan_scene.h"

namespace svulkan
{
VulkanScene::VulkanScene(vk::PhysicalDevice physicalDevice, vk::Device device,
                         vk::DescriptorPool descriptorPool,
                         vk::DescriptorSetLayout descriptorLayout): mDevice(device) {
  mUBO = std::make_unique<VulkanBufferData>(physicalDevice, device, sizeof(SceneUBO),
                                            vk::BufferUsageFlagBits::eUniformBuffer);
  mDescriptorSet = std::move(
      device.allocateDescriptorSetsUnique({descriptorPool, 1, &descriptorLayout}).front());
  updateDescriptorSets(device, mDescriptorSet.get(),
                       {{vk::DescriptorType::eUniformBuffer, mUBO->mBuffer.get(), vk::BufferView()}}, {}, 0);
}

void VulkanScene::updateUBO(SceneUBO const &ubo) {
  copyToDevice(mDevice, mUBO->getMemory(), ubo);
}
}
