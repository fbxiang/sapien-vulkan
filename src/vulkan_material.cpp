#include "vulkan_material.h"


namespace svulkan
{

vk::UniqueDescriptorSetLayout VulkanMaterial::CreateDescriptorSetLayout(vk::Device device) {
  return createDescriptorSetLayout(
      device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment}});
}

vk::UniqueShaderModule VulkanMaterial::LoadVertexShader(vk::Device device) {
  return createShaderModule(device, "gbuffer.vert.spv");
}

vk::UniqueShaderModule VulkanMaterial::LoadFragmentShader(vk::Device device) {
  return createShaderModule(device, "gbuffer.frag.spv");
}

VulkanMaterial::VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                                               vk::DescriptorPool descriptorPool,
                                               vk::DescriptorSetLayout descriptorLayout)
    : mUBO(physicalDevice, device, sizeof(PBRMaterialUBO),
           vk::BufferUsageFlagBits::eUniformBuffer) {
  mDescriptorSet = std::move(
      device.allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(descriptorPool, 1, &descriptorLayout))
      .front());

  updateDescriptorSets(device, mDescriptorSet.get(),
                       {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}}, {}, 0);
}

void VulkanMaterial::setProperties(vk::Device device, PBRMaterialUBO const &data) {
  copyToDevice<PBRMaterialUBO>(device, mUBO.mMemory.get(), data);
}

}
