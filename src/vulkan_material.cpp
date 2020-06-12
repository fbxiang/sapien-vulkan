#include "vulkan_material.h"

namespace svulkan
{
VulkanMaterial::VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                                               vk::DescriptorPool descriptorPool,
                               vk::DescriptorSetLayout descriptorLayout) 
    : mDevice(device), mUBO(physicalDevice, device, sizeof(PBRMaterialUBO),
           vk::BufferUsageFlagBits::eUniformBuffer) {
  mDescriptorSet = std::move(
      device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(descriptorPool, 1, &descriptorLayout))
      .front());

  updateDescriptorSets(device, mDescriptorSet.get(),
                       {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}}, {}, 0);
}

void VulkanMaterial::setProperties(PBRMaterialUBO const &data) {
  copyToDevice<PBRMaterialUBO>(mDevice, mUBO.mMemory.get(), data);
}

}
