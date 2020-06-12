#include "vulkan_image.h"

namespace svulkan
{

VulkanImageData::VulkanImageData(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Format format,
                                 vk::Extent2D const &extent, uint32_t mipLevels,
                                 vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                 vk::ImageLayout initialLayout, vk::MemoryPropertyFlags memoryProperties,
                                 vk::ImageAspectFlags aspectMask)
    : mFormat(format), mExtent(extent), mMipLevels(mipLevels), mMemoryProperties(memoryProperties)
{

  vk::ImageCreateInfo imageInfo (
      vk::ImageCreateFlags(), vk::ImageType::e2D, mFormat,
      vk::Extent3D(extent, 1), mipLevels, 1, vk::SampleCountFlagBits::e1, tiling,
      usage | vk::ImageUsageFlagBits::eSampled, vk::SharingMode::eExclusive, 0, nullptr, initialLayout);

  mImage = device.createImageUnique(imageInfo);
  mMemory = allocateMemory(device, physicalDevice.getMemoryProperties(),
                           device.getImageMemoryRequirements(mImage.get()), memoryProperties);
  device.bindImageMemory(mImage.get(), mMemory.get(), 0);
  vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                        vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
  vk::ImageViewCreateInfo imageViewInfo(vk::ImageViewCreateFlags(), mImage.get(), vk::ImageViewType::e2D,
                                        mFormat, componentMapping,
                                        vk::ImageSubresourceRange(aspectMask, 0, mipLevels, 0, 1));

  mImageView = device.createImageViewUnique(imageViewInfo);
}

}
