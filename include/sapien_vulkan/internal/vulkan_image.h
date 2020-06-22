#pragma once
#include "sapien_vulkan/common/glm_common.h"
#include "vulkan_buffer.h"
#include "vulkan_util.h"

namespace svulkan {

struct VulkanImageData {
  vk::Format mFormat;
  vk::UniqueImage mImage;
  vk::UniqueDeviceMemory mMemory;
  vk::UniqueImageView mImageView;
  vk::Extent2D mExtent;
  uint32_t mMipLevels;

  vk::MemoryPropertyFlags mMemoryProperties;
  
  VulkanImageData(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Format format,
                  vk::Extent2D const &extent, uint32_t mipLevels,
                  vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                  vk::ImageLayout initialLayout, vk::MemoryPropertyFlags memoryProperties,
                  vk::ImageAspectFlags aspectMask);

  template <typename DataType>
  std::vector<DataType> download(vk::PhysicalDevice physicalDevice, vk::Device device,
                                 vk::CommandPool commandPool, vk::Queue queue, size_t size) const {
    vk::ImageLayout sourceLayout;
    vk::AccessFlags sourceAccessFlag1;
    vk::AccessFlags sourceAccessFlag2;
    vk::PipelineStageFlags sourceStage;
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;

    // only support float textures
    static_assert(sizeof(DataType) == 4);

    if (mFormat == vk::Format::eR32G32B32A32Sfloat) {
      sourceLayout = vk::ImageLayout::eColorAttachmentOptimal;
      sourceAccessFlag1 = vk::AccessFlagBits::eColorAttachmentWrite;
      sourceAccessFlag2 = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
      sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      aspect = vk::ImageAspectFlagBits::eColor;
    } else if (mFormat == vk::Format::eD32Sfloat) {
      sourceLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      sourceAccessFlag1 = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      sourceAccessFlag2 = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
      sourceStage = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
      aspect = vk::ImageAspectFlagBits::eDepth;
    } else if (mFormat == vk::Format::eR32G32B32A32Uint){
      sourceLayout = vk::ImageLayout::eColorAttachmentOptimal;
      sourceAccessFlag1 = vk::AccessFlagBits::eColorAttachmentWrite;
      sourceAccessFlag2 = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
      sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      aspect = vk::ImageAspectFlagBits::eColor;
    } else {
      throw std::runtime_error("This image format does not support download");
    }

    std::vector<DataType> output;

    if (!((mMemoryProperties & vk::MemoryPropertyFlagBits::eHostVisible) &&
          (mMemoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))) {

      VulkanBufferData stagingBuffer(physicalDevice, device, size, vk::BufferUsageFlagBits::eTransferDst);

      // copy image to buffer
      OneTimeSubmit(device, commandPool, queue, [&](vk::CommandBuffer commandBuffer) {
        transitionImageLayout(commandBuffer, mImage.get(), mFormat,
                              sourceLayout, vk::ImageLayout::eTransferSrcOptimal,
                              sourceAccessFlag1, vk::AccessFlagBits::eTransferRead,
                              sourceStage, vk::PipelineStageFlagBits::eTransfer,
                              aspect, mMipLevels);
        vk::BufferImageCopy copyRegion(0, mExtent.width, mExtent.height, {aspect, 0, 0, 1},
                                       {0, 0, 0}, vk::Extent3D(mExtent, 1));
        commandBuffer.copyImageToBuffer(mImage.get(), vk::ImageLayout::eTransferSrcOptimal,
                                        stagingBuffer.getBuffer(), copyRegion);
        transitionImageLayout(commandBuffer, mImage.get(), mFormat,
                              vk::ImageLayout::eTransferSrcOptimal, sourceLayout,
                              vk::AccessFlagBits::eTransferRead,
                              sourceAccessFlag2,
                              vk::PipelineStageFlagBits::eTransfer,
                              sourceStage,
                              aspect, mMipLevels);
      });

      // copy buffer to host memory
      output.resize(size / sizeof(DataType));
      const char *memory = static_cast<const char *>(device.mapMemory(stagingBuffer.getMemory(), 0, size));
      memcpy(output.data(), memory, size);
      device.unmapMemory(stagingBuffer.getMemory());

    } else {
      vk::ImageSubresource subResource(aspect, 0, 0);
      vk::SubresourceLayout subresourceLayout = device.getImageSubresourceLayout(mImage.get(), subResource);

      output.resize(size / sizeof(DataType));
      const char *memory = static_cast<const char *>(device.mapMemory(mMemory.get(), 0, size));
      memcpy(output.data(), memory + subresourceLayout.offset, size);
      device.unmapMemory(mMemory.get());
    }
    return output;
  }
};

}
