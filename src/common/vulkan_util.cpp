#include "vulkan_util.h"
#include "fs.h"

namespace svulkan {
static uint32_t findMemoryType(vk::PhysicalDeviceMemoryProperties const &memoryProperties, uint32_t typeBits,
                               vk::MemoryPropertyFlags requirementsMask) {
  uint32_t typeIndex = ~0u;
  for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) &&
        ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)) {
      typeIndex = i;
      break;
    }
    typeBits >>= 1;
  }
  if (typeIndex != ~0u)
    return typeIndex;
  throw std::runtime_error("No memory type found!");
}

vk::UniqueShaderModule createShaderModule(vk::Device device, const std::string &filename) {
  auto code = readFile(filename);
  return device.createShaderModuleUnique(
      vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(),
                                 code.size(),
                                 reinterpret_cast<uint32_t*>(code.data())));
}

vk::UniqueDeviceMemory allocateMemory(vk::Device device,
                                      vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                                      vk::MemoryRequirements const &memoryRequirements,
                                      vk::MemoryPropertyFlags memoryPropertyFlags) {
  uint32_t memoryTypeIndex =
      findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

  return device.allocateMemoryUnique(vk::MemoryAllocateInfo(memoryRequirements.size, memoryTypeIndex));
}

void transitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format,
                           vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout,
                           vk::AccessFlags sourceAccessMask, vk::AccessFlags destAccessMask,
                           vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags destStage,
                           vk::ImageAspectFlags aspectMask, uint32_t mipLevels) {
  // TODO: do a warning if something seems off

  vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, mipLevels, 0, 1);
  vk::ImageMemoryBarrier barrier(sourceAccessMask, destAccessMask, oldImageLayout, newImageLayout,
                                 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image, imageSubresourceRange);
  commandBuffer.pipelineBarrier(sourceStage, destStage, {}, nullptr, nullptr, barrier);
}

}
