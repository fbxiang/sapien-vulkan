#include "sapien_vulkan/camera.h"

namespace svulkan {

Camera::Camera(vk::PhysicalDevice physicalDevice, vk::Device device,
               vk::DescriptorPool descriptorPool, vk::DescriptorSetLayout descriptorLayout)
    : mDevice(device),
      mUBO(physicalDevice, device, sizeof(CameraUBO), vk::BufferUsageFlagBits::eUniformBuffer) {

  mDescriptorSet = std::move(device
                                 .allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(
                                     descriptorPool, 1, &descriptorLayout))
                                 .front());

  updateDescriptorSets(
      device, mDescriptorSet.get(),
      {{vk::DescriptorType::eUniformBuffer, mUBO.mBuffer.get(), vk::BufferView()}}, {}, 0);
}

void Camera::updateUBO() {
  copyToDevice<CameraUBO>(mDevice, mUBO.mMemory.get(),
                          {getViewMat(), getProjectionMat(), glm::inverse(getViewMat()),
                           glm::inverse(getProjectionMat()), userData});
}

glm::mat4 Camera::getModelMat() const {
  glm::mat4 t = glm::toMat4(rotation);
  t[3][0] = position.x;
  t[3][1] = position.y;
  t[3][2] = position.z;
  return t;
}

glm::mat4 Camera::getViewMat() const { return glm::inverse(getModelMat()); }

glm::mat4 Camera::getProjectionMat() const {
  glm::mat4 proj;
  if (ortho) {
    proj = glm::ortho(-scaling * aspect, scaling * aspect, -scaling, scaling, near, far);
  } else {
    proj = glm::perspective(fovy, aspect, near, far);
  }
  proj[1][1] *= -1;
  return proj;
}

} // namespace svulkan
