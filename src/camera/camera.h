#pragma once
#include "common/glm_common.h"
#include "common/vulkan.h"

namespace svulkan
{
struct CameraUBO {
  glm::mat4 viewMatrix;
  glm::mat4 projMatrix;
  glm::mat4 viewMatrixInverse;
  glm::mat4 projMatrixInverse;
};

struct Camera {
  glm::vec3 position;
  glm::quat rotation;
  float near = 0.1f;
  float far = 1000.f;
  float fovy = glm::radians(45.f);
  float aspect = 1.f;

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
