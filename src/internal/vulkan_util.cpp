#include "sapien_vulkan/internal/vulkan_util.h"
#include "sapien_vulkan/common/fs.h"
#include "sapien_vulkan/internal/vulkan_texture.h"
#include <numeric>

namespace svulkan {
static uint32_t findMemoryType(vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                               uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask) {
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
  throw std::runtime_error("find memory failed: no memory type exists");
}

vk::UniqueShaderModule createShaderModule(vk::Device device, const std::string &filename) {
  std::vector<char> code = readFile(filename);
  assert(code.size() % sizeof(uint32_t) == 0);
  std::vector<uint32_t> shaderCode(code.size() / sizeof(uint32_t));
  std::memcpy(shaderCode.data(), code.data(), shaderCode.size() * sizeof(uint32_t));

  try {
    return device.createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), code.size(), shaderCode.data()));
  } catch (const std::runtime_error &error) {
    throw std::runtime_error("Failed to load " + filename);
  }
}

vk::UniqueDeviceMemory allocateMemory(vk::Device device,
                                      vk::PhysicalDeviceMemoryProperties const &memoryProperties,
                                      vk::MemoryRequirements const &memoryRequirements,
                                      vk::MemoryPropertyFlags memoryPropertyFlags) {
  uint32_t memoryTypeIndex =
      findMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

  return device.allocateMemoryUnique(
      vk::MemoryAllocateInfo(memoryRequirements.size, memoryTypeIndex));
}

void transitionImageLayout(vk::CommandBuffer commandBuffer, vk::Image image, vk::Format format,
                           vk::ImageLayout oldImageLayout, vk::ImageLayout newImageLayout,
                           vk::AccessFlags sourceAccessMask, vk::AccessFlags destAccessMask,
                           vk::PipelineStageFlags sourceStage, vk::PipelineStageFlags destStage,
                           vk::ImageAspectFlags aspectMask, uint32_t mipLevels) {
  vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, mipLevels, 0, 1);
  vk::ImageMemoryBarrier barrier(sourceAccessMask, destAccessMask, oldImageLayout, newImageLayout,
                                 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image,
                                 imageSubresourceRange);
  commandBuffer.pipelineBarrier(sourceStage, destStage, {}, nullptr, nullptr, barrier);
}

void updateDescriptorSets(
    vk::Device device, vk::DescriptorSet descriptorSet,
    std::vector<std::tuple<vk::DescriptorType, vk::Buffer, vk::BufferView>> const &bufferData,
    std::vector<std::shared_ptr<VulkanTextureData>> const &textureData, uint32_t bindingOffset) {

  std::vector<vk::DescriptorBufferInfo> bufferInfos;
  bufferInfos.reserve(bufferData.size());

  std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
  writeDescriptorSets.reserve(bufferData.size() + textureData.size() ? 1 : 0);

  uint32_t dstBinding = bindingOffset;
  for (auto const &bd : bufferData) {
    bufferInfos.push_back(vk::DescriptorBufferInfo(std::get<1>(bd), 0, VK_WHOLE_SIZE));
    writeDescriptorSets.push_back(
        vk::WriteDescriptorSet(descriptorSet, dstBinding++, 0, 1, std::get<0>(bd), nullptr,
                               &bufferInfos.back(), std::get<2>(bd) ? &std::get<2>(bd) : nullptr));
  }

  std::vector<vk::DescriptorImageInfo> imageInfos;
  for (auto const &tex : textureData) {
    imageInfos.push_back(vk::DescriptorImageInfo(tex->mTextureSampler.get(),
                                                 tex->mImageData->mImageView.get(),
                                                 vk::ImageLayout::eShaderReadOnlyOptimal));
  }
  if (imageInfos.size()) {
    writeDescriptorSets.push_back(
        vk::WriteDescriptorSet(descriptorSet, dstBinding, 0, imageInfos.size(),
                               vk::DescriptorType::eCombinedImageSampler, imageInfos.data()));
  }
  device.updateDescriptorSets(writeDescriptorSets, nullptr);
}

vk::UniqueCommandBuffer createCommandBuffer(vk::Device device, vk::CommandPool commandPool,
                                            vk::CommandBufferLevel level) {
  return std::move(
      device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(commandPool, level, 1))
          .front());
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
    bindings[i] =
        vk::DescriptorSetLayoutBinding(static_cast<uint32_t>(i), std::get<0>(bindingData[i]),
                                       std::get<1>(bindingData[i]), std::get<2>(bindingData[i]));
  }
  return device.createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo(
      flags, static_cast<uint32_t>(bindings.size()), bindings.data()));
}

vk::UniqueDescriptorPool
createDescriptorPool(vk::Device device, std::vector<vk::DescriptorPoolSize> const &poolSizes) {
  assert(!poolSizes.empty());
  uint32_t maxSets = std::accumulate(
      poolSizes.begin(), poolSizes.end(), 0,
      [](uint32_t sum, vk::DescriptorPoolSize const &dps) { return sum + dps.descriptorCount; });
  assert(maxSets > 0);

  vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
      vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, maxSets,
      static_cast<uint32_t>(poolSizes.size()), poolSizes.data());
  return device.createDescriptorPoolUnique(descriptorPoolCreateInfo);
}

vk::UniqueFramebuffer createFramebuffer(vk::Device device, vk::RenderPass renderPass,
                                        std::vector<vk::ImageView> const &colorImageViews,
                                        vk::ImageView depthImageView, vk::Extent2D const &extent) {
  std::vector<vk::ImageView> imageViews = colorImageViews;
  if (depthImageView) {
    imageViews.push_back(depthImageView);
  }
  vk::FramebufferCreateInfo info({}, renderPass, imageViews.size(), imageViews.data(),
                                 extent.width, extent.height, 1);
  return device.createFramebufferUnique(info);
}

} // namespace svulkan
