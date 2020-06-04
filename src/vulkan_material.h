#pragma once

#include "common/glm_common.h"
#include "common/vulkan.h"

namespace svulkan
{

struct PBRMaterialUBO {
  glm::vec4 baseColor;
  float specular;
  float roughness;
  float metallic;
  float additionalTransparency;
};

struct VulkanMaterial {
  static vk::UniquePipeline &pipeline;
  static vk::UniqueDescriptorSetLayout CreateDescriptorSetLayout(vk::Device device);
  static vk::UniqueShaderModule LoadVertexShader(vk::Device device);
  static vk::UniqueShaderModule LoadFragmentShader(vk::Device device);
  static std::shared_ptr<VulkanMaterial> CreateMaterial(vk::PhysicalDevice physicalDevice,
                                                        vk::Device device,
                                                        vk::DescriptorPool descriptorPool,
                                                        vk::DescriptorSetLayout descriptorLayout);
  VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                 vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout);
  void setProperties(vk::Device device, PBRMaterialUBO const &data);
  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;
  
  VulkanTextureData *mDiffuseMap;
  VulkanTextureData *mSpecularMap;
  VulkanTextureData *mNormalMap;
};

}
