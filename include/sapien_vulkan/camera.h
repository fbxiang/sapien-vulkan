#pragma once
#include "common/glm_common.h"
#include "internal/vulkan.h"
#include "uniform_buffers.h"

namespace svulkan
{
struct Camera {
  glm::vec3 position = {};
  glm::quat rotation = {};
  float near = 0.1f;
  float far = 1000.f;
  float fovy = glm::radians(45.f);
  float aspect = 1.f;

  float scaling = 1.f;
  bool ortho = false;

  vk::Device mDevice;
  VulkanBufferData mUBO;
  vk::UniqueDescriptorSet mDescriptorSet;

  Camera(vk::PhysicalDevice physicalDevice, vk::Device device, vk::DescriptorPool descriptorPool,
         vk::DescriptorSetLayout descriptorLayout);

  void updateUBO();

  glm::mat4 getModelMat() const;
  glm::mat4 getViewMat() const;
  glm::mat4 getProjectionMat() const;
};

}
