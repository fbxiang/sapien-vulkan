#include "vulkan_material.h"

namespace svulkan
{
VulkanMaterial::VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                               vk::DescriptorPool descriptorPool,
                               vk::DescriptorSetLayout descriptorLayout,
                               std::shared_ptr<VulkanTextureData> defaultTexture)
    : mDevice(device), mUBO(physicalDevice, device, sizeof(PBRMaterialUBO),
                            vk::BufferUsageFlagBits::eUniformBuffer),
      mDiffuseMap(defaultTexture), mSpecularMap(defaultTexture), 
      mNormalMap(defaultTexture), mHeightMap(defaultTexture) {
  mDescriptorSet = std::move(
      device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(descriptorPool, 1, &descriptorLayout))
      .front());
}

void VulkanMaterial::setProperties(PBRMaterialUBO const &data) {

#if !defined(NDEBUG)
  assert(mInitialized);
#endif

  copyToDevice<PBRMaterialUBO>(mDevice, mUBO.mMemory.get(), data);
}

void VulkanMaterial::updateDescriptorSets() {
  svulkan::updateDescriptorSets(
      mDevice, mDescriptorSet.get(),
      {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}},
      {mDiffuseMap , mSpecularMap, mNormalMap, mHeightMap},
      0);
#if !defined(NDEBUG)
  mInitialized = true;
#endif
}

}
