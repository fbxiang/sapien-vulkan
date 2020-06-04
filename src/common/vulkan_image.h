#pragma once
#include "glm_common.h"
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


  // TODO: color attachment only for now
  template <typename DataType>
  std::vector<DataType> download(vk::PhysicalDevice physicalDevice, vk::Device device,
                                 vk::CommandPool commandPool, vk::Queue queue, size_t size) const {

    std::vector<DataType> output;

    if (!((mMemoryProperties & vk::MemoryPropertyFlagBits::eHostVisible) &&
          (mMemoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))) {
      vk::ImageCreateInfo imageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, mFormat,
                                          vk::Extent3D(mExtent, 1), 1, 1, vk::SampleCountFlagBits::e1,
                                          vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eTransferDst,
                                          vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);
      // create "staging image"
      vk::UniqueImage dstImage = device.createImageUnique(imageCreateInfo);
      vk::UniqueDeviceMemory dstMemory = allocateMemory(
          device, physicalDevice.getMemoryProperties(), device.getImageMemoryRequirements(dstImage.get()),
          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
      device.bindImageMemory(*dstImage, *dstMemory, 0);

      OneTimeSubmit(device, commandPool, queue, [&](vk::CommandBuffer commandBuffer) {
        transitionImageLayout(commandBuffer, mImage.get(), mFormat,
                              vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal,
                              vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eTransferRead,
                              vk::PipelineStageFlagBits::eColorAttachmentOutput,
                              vk::PipelineStageFlagBits::eTransfer,
                              vk::ImageAspectFlagBits::eColor, mMipLevels);
        transitionImageLayout(commandBuffer, dstImage.get(), mFormat,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                              vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite,
                              vk::PipelineStageFlagBits::eTopOfPipe,
                              vk::PipelineStageFlagBits::eTransfer,
                              vk::ImageAspectFlagBits::eColor, 1);
        vk::ImageCopy imageCopy(
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            vk::Offset3D(0, 0, 0),
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            vk::Offset3D(0, 0, 0), vk::Extent3D(mExtent, 1));
        commandBuffer.copyImage(mImage.get(), vk::ImageLayout::eTransferSrcOptimal, dstImage.get(), vk::ImageLayout::eTransferDstOptimal, imageCopy);
        transitionImageLayout(commandBuffer, mImage.get(), mFormat,
                              vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal,
                              vk::AccessFlagBits::eTransferRead,
                              vk::AccessFlagBits::eColorAttachmentRead |
                              vk::AccessFlagBits::eColorAttachmentWrite,

                              vk::PipelineStageFlagBits::eTransfer,
                              vk::PipelineStageFlagBits::eFragmentShader,
                              vk::ImageAspectFlagBits::eColor, mMipLevels);
        transitionImageLayout(commandBuffer, dstImage.get(), mFormat,
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral,
                              vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eHostRead,
                              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost,
                              vk::ImageAspectFlagBits::eColor);
      });

      vk::ImageSubresource subResource(vk::ImageAspectFlagBits::eColor, 0, 0);
      vk::SubresourceLayout subresourceLayout = device.getImageSubresourceLayout(dstImage.get(), subResource);

      output.resize(size / sizeof(DataType));
      const char *memory = static_cast<const char *>(device.mapMemory(dstMemory.get(), 0, size));
      memcpy(output.data(), memory + subresourceLayout.offset, size);
      device.unmapMemory(dstMemory.get());
    } else {
      vk::ImageSubresource subResource(vk::ImageAspectFlagBits::eColor, 0, 0);
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
