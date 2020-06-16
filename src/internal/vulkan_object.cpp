#include "sapien_vulkan/internal/vulkan_object.h"

namespace svulkan
{

VulkanObject::VulkanObject(vk::PhysicalDevice physicalDevice, vk::Device device,
                           vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout)
    : mDevice(device),
      mUBO(physicalDevice, device, sizeof(ObjectUBO), vk::BufferUsageFlagBits::eUniformBuffer) {

  mDescriptorSet = std::move(
      device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(descriptorPool, 1, &descriptorLayout))
      .front());

  updateDescriptorSets(device, mDescriptorSet.get(),
                       {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}}, {}, 0);
}

void VulkanObject::setMesh(std::shared_ptr<VulkanMesh> mesh) { mMesh = mesh; }

void VulkanObject::setMaterial(std::shared_ptr<VulkanMaterial> material) { mMaterial = material; }

}
