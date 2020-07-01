#pragma once

#include "sapien_vulkan/common/glm_common.h"
#include "sapien_vulkan/uniform_buffers.h"
#include "vulkan.h"
#include <array>

namespace svulkan {

class VulkanMaterial {
  vk::Device mDevice;
  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;

  std::shared_ptr<VulkanTextureData> mDiffuseMap;
  std::shared_ptr<VulkanTextureData> mSpecularMap;
  std::shared_ptr<VulkanTextureData> mNormalMap;
  std::shared_ptr<VulkanTextureData> mHeightMap;

  PBRMaterialUBO mMaterial;

public:
  VulkanMaterial(vk::PhysicalDevice physicalDevice, vk::Device device,
                 vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout,
                 std::shared_ptr<VulkanTextureData> defaultTexture);

  void setProperties(PBRMaterialUBO const &data);
  inline PBRMaterialUBO const &getProperties() const { return mMaterial; };

  void setDiffuseTexture(std::shared_ptr<VulkanTextureData> tex);
  void setSpecularTexture(std::shared_ptr<VulkanTextureData> tex);
  void setNormalTexture(std::shared_ptr<VulkanTextureData> tex);
  void setHeightTexture(std::shared_ptr<VulkanTextureData> tex);

  inline auto getDiffuseTexture() const { return mDiffuseMap; };
  inline auto getSpecularTexture() const { return mSpecularMap; };
  inline auto getNormalTexture() const { return mNormalMap; };
  inline auto getHeightTexture() const { return mHeightMap; };

  inline vk::DescriptorSet getDescriptorSet() const { return mDescriptorSet.get(); }
};

} // namespace svulkan
