#pragma once
#include <vulkan/vulkan.hpp>
#include "sapien_vulkan/common/log.h"

namespace svulkan {
/** Load shader file to create shader module
 */
vk::UniqueShaderModule createShaderModule(vk::Device device, const std::string &filename);

/** Allocate memory given requirements */
vk::UniqueDeviceMemory allocateMemory(vk::Device device,
                                      vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                                      vk::MemoryRequirements const &memoryRequirements,
                                      vk::MemoryPropertyFlags memoryPropertyFlags);

/** Copy an array of data to device with given stride */
template<class T>
void copyToDevice(vk::Device device, vk::DeviceMemory memory, T const* pData, size_t count, size_t stride = sizeof(T)) {
  log::check(sizeof(T) <= stride, "copyToDevice failed: stride is smaller than data size");

  uint8_t *deviceData = static_cast<uint8_t*>(device.mapMemory(memory, 0, count * stride));

  if (stride == sizeof(T)) {
    memcpy(deviceData, pData, count * sizeof(T));
  } else {
    for (size_t i = 0; i < count; i++) {
      memcpy(deviceData, &pData[i], sizeof(T));
      deviceData += stride;
    }
  }
  device.unmapMemory(memory);
}

/** Copy a single block of data to device */
template<class T>
void copyToDevice(vk::Device device, vk::DeviceMemory memory, T const &data) {
  copyToDevice(device, memory, &data, 1);
}

template <typename Func>
void OneTimeSubmitNoWait(vk::CommandBuffer commandBuffer, vk::Queue queue, Func const &func) {
  commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
  func(commandBuffer);
  commandBuffer.end();
  queue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &commandBuffer), nullptr);
}

template <typename Func>
void OneTimeSubmit(vk::CommandBuffer commandBuffer, vk::Queue queue, Func const &func) {
  OneTimeSubmitNoWait(commandBuffer, queue, func);
  queue.waitIdle();
}

template <typename Func>
void OneTimeSubmit(vk::Device device, vk::CommandPool commandPool, vk::Queue queue, Func const &func) {
  vk::UniqueCommandBuffer commandBuffer =
      std::move(device
                .allocateCommandBuffersUnique(
                    vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1))
                .front());
  OneTimeSubmitNoWait(commandBuffer.get(), queue, func);
  queue.waitIdle();
}

void transitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format,
                           vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout,
                           vk::AccessFlags sourceAccessMask, vk::AccessFlags destAccessMask,
                           vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags destStage,
                           vk::ImageAspectFlags aspectMask, uint32_t mipLevels=1);


void updateDescriptorSets(
    vk::Device device, vk::DescriptorSet descriptorSet,
    std::vector<std::tuple<vk::DescriptorType, vk::Buffer, vk::BufferView>> const &bufferData,
    std::vector<std::shared_ptr<struct VulkanTextureData>> const &textureData, uint32_t bindingOffset); 

vk::UniqueCommandBuffer createCommandBuffer(vk::Device device, vk::CommandPool commandPool,
                                            vk::CommandBufferLevel level);


vk::UniqueDescriptorSetLayout createDescriptorSetLayout(
    vk::Device device,
    std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> const &bindingData,
    vk::DescriptorSetLayoutCreateFlags flags = {}); 

vk::UniqueDescriptorPool createDescriptorPool(vk::Device device,
                                              std::vector<vk::DescriptorPoolSize> const &poolSizes);

vk::UniqueFramebuffer createFramebuffer(vk::Device device, vk::RenderPass renderPass,
                                        std::vector<vk::ImageView> const&colorImageViews,
                                        vk::ImageView depthImageView, vk::Extent2D const&extent);

}
