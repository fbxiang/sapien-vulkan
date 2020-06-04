#include "vulkan_util.h"
#include "fs.h"
#include "vulkan_texture.h"

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


void updateDescriptorSets(
    vk::Device device, vk::DescriptorSet descriptorSet,
    std::vector<std::tuple<vk::DescriptorType, vk::Buffer, vk::BufferView>> const &bufferData,
    std::vector<struct VulkanTextureData> const &textureData, uint32_t bindingOffset) {

  std::vector<vk::DescriptorBufferInfo> bufferInfos;
  bufferInfos.reserve(bufferData.size());

  std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
  writeDescriptorSets.reserve(bufferData.size() + (textureData.empty() ? 0 : 1));

  uint32_t dstBinding = bindingOffset;
  for (auto const &bd : bufferData) {
    bufferInfos.push_back(vk::DescriptorBufferInfo(std::get<1>(bd), 0, VK_WHOLE_SIZE));
    writeDescriptorSets.push_back(vk::WriteDescriptorSet(descriptorSet, dstBinding++, 0, 1, std::get<0>(bd),
                                                         nullptr, &bufferInfos.back(),
                                                         std::get<2>(bd) ? &std::get<2>(bd) : nullptr));
  }
  if (!textureData.empty()) {
    vk::DescriptorImageInfo imageInfo(*textureData[0].mTextureSampler, *textureData[0].mImageData->mImageView,
                                      vk::ImageLayout::eShaderReadOnlyOptimal);
    writeDescriptorSets.push_back(vk::WriteDescriptorSet(descriptorSet, dstBinding, 0, 1,
                                                         vk::DescriptorType::eCombinedImageSampler, &imageInfo,
                                                         nullptr, nullptr));
  }
  device.updateDescriptorSets(writeDescriptorSets, nullptr);
}

vk::UniqueCommandBuffer createCommandBuffer(vk::Device device, vk::CommandPool commandPool,
                                            vk::CommandBufferLevel level) {
  return std::move
      (device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(commandPool, level, 1)).front());
}



vk::UniqueDescriptorSetLayout createDescriptorSetLayout(
    vk::Device device,
    std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> const &bindingData,
    vk::DescriptorSetLayoutCreateFlags flags) {
  std::vector<vk::DescriptorSetLayoutBinding> bindings(bindingData.size());

#if !defined(NDEBUG)
  assert(bindingData.size() <= 0xfffffffful);
#endif

  for (size_t i = 0; i < bindingData.size(); i++) {
    bindings[i] = vk::DescriptorSetLayoutBinding(static_cast<uint32_t>(i), std::get<0>(bindingData[i]),
                                                 std::get<1>(bindingData[i]), std::get<2>(bindingData[i]));
  }
  return device.createDescriptorSetLayoutUnique(
      vk::DescriptorSetLayoutCreateInfo(flags, static_cast<uint32_t>(bindings.size()), bindings.data()));
}

}
