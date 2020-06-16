#include "sapien_vulkan/internal/vulkan_material.h"

namespace svulkan
{
VulkanMaterial::VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                               vk::DescriptorPool descriptorPool,
                               vk::DescriptorSetLayout descriptorLayout,
                               std::shared_ptr<VulkanTextureData> defaultTexture)
    : mDevice(device), mUBO(physicalDevice, device, sizeof(PBRMaterialUBO),
                            vk::BufferUsageFlagBits::eUniformBuffer) {
  mDescriptorSet = std::move(
      device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(descriptorPool, 1, &descriptorLayout))
      .front());
  svulkan::updateDescriptorSets(
      mDevice, mDescriptorSet.get(),
      {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}},
      { defaultTexture, defaultTexture, defaultTexture, defaultTexture }, 0);
}

void VulkanMaterial::setProperties(PBRMaterialUBO const &data) {
  copyToDevice<PBRMaterialUBO>(mDevice, mUBO.mMemory.get(), data);
}


void VulkanMaterial::setDiffuseTexture(std::shared_ptr<VulkanTextureData> tex) {
  mDiffuseMap = tex;
  svulkan::updateDescriptorSets(mDevice, mDescriptorSet.get(), {}, {mDiffuseMap}, 1);
}
void VulkanMaterial::setSpecularTexture(std::shared_ptr<VulkanTextureData> tex) {
  mSpecularMap = tex;
  svulkan::updateDescriptorSets(mDevice, mDescriptorSet.get(), {}, {mSpecularMap}, 2);
}
void VulkanMaterial::setNormalTexture(std::shared_ptr<VulkanTextureData> tex) {
  mNormalMap = tex;
  svulkan::updateDescriptorSets(mDevice, mDescriptorSet.get(), {}, {mNormalMap}, 3);
}
void VulkanMaterial::setHeightTexture(std::shared_ptr<VulkanTextureData> tex) {
  mHeightMap = tex;
  svulkan::updateDescriptorSets(mDevice, mDescriptorSet.get(), {}, {mHeightMap}, 4);
}

// void VulkanMaterial::updateDescriptorSets() {
//   svulkan::updateDescriptorSets(
//       mDevice, mDescriptorSet.get(),
//       {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}},
//       {mDiffuseMap , mSpecularMap, mNormalMap, mHeightMap},
//       0);
// #if !defined(NDEBUG)
//   mInitialized = true;
// #endif
// }

}
