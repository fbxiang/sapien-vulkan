#pragma once
#include "vulkan_image.h"

namespace svulkan {

// TODO: support mipmaps
struct VulkanTextureData {
  vk::Format mFormat;
  vk::Extent2D mExtent;
  std::unique_ptr<VulkanImageData> mImageData;
  vk::UniqueSampler mTextureSampler;

  VulkanTextureData(vk::PhysicalDevice physicalDevice, vk::Device device, const vk::Extent2D &extent,
                    vk::ImageTiling tiling = vk::ImageTiling::eLinear, vk::ImageUsageFlags usage = {},
                    vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal,
                    vk::FormatFeatureFlags formatFeatureFlags = {}, bool anisotropyEnable = false);

  template <typename ImageGenerator>
  void setImage(vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandBuffer commandBuffer,
                const ImageGenerator &imageGenerator) {

    VkDeviceSize dataSize = device.getImageMemoryRequirements(*mImageData->mImage).size;
    VulkanBufferData stagingBuffer(physicalDevice, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
    void *data = device.mapMemory(*stagingBuffer.mMemory, 0, dataSize);
    imageGenerator(data, mExtent);
    device.unmapMemory(*stagingBuffer.mMemory);

    transitionImageLayout(commandBuffer, mImageData->mImage.get(), mFormat,
                          vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                          vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite,
                          vk::PipelineStageFlagBits::eTopOfPipe,
                          vk::PipelineStageFlagBits::eTransfer,
                          vk::ImageAspectFlagBits::eColor, 1);

    vk::BufferImageCopy copyRegion(0, mExtent.width, mExtent.height,
                                   vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                   vk::Offset3D(0, 0, 0), vk::Extent3D(mExtent, 1));

    commandBuffer.copyBufferToImage(*stagingBuffer.mBuffer, *mImageData->mImage,
                                    vk::ImageLayout::eTransferDstOptimal, copyRegion);


    transitionImageLayout(commandBuffer, mImageData->mImage.get(), mFormat,
                          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                          vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
                          vk::PipelineStageFlagBits::eTransfer,
                          vk::PipelineStageFlagBits::eFragmentShader,
                          vk::ImageAspectFlagBits::eColor, 1);
  }

};

}
