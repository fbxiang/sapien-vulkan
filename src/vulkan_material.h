#pragma once

#include "common/glm_common.h"
#include "common/vulkan.h"
#include "uniform_buffers.h"
#include <array>

namespace svulkan
{

class VulkanMaterial {
  vk::Device mDevice;
  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;
  
  std::shared_ptr<VulkanTextureData> mDiffuseMap;
  std::shared_ptr<VulkanTextureData> mSpecularMap;
  std::shared_ptr<VulkanTextureData> mNormalMap;
  std::shared_ptr<VulkanTextureData> mHeightMap;

 public:
  VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                 vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout,
                 std::shared_ptr<VulkanTextureData> defaultTexture);
  void setProperties(PBRMaterialUBO const &data);

  void setDiffuseTexture(std::shared_ptr<VulkanTextureData> tex);
  void setSpecularTexture(std::shared_ptr<VulkanTextureData> tex);
  void setNormalTexture(std::shared_ptr<VulkanTextureData> tex);
  void setHeightTexture(std::shared_ptr<VulkanTextureData> tex);

  inline auto getDiffuseTexture() const { return mDiffuseMap; };
  inline auto getSpecularTexture() const {return mSpecularMap; };
  inline auto getNormalTexture() const {return mNormalMap;};
  inline auto getHeightTexture() const { return mHeightMap; };

  inline vk::DescriptorSet getDescriptorSet() const { return mDescriptorSet.get(); }
};

}
