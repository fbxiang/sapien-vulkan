#include "vulkan_texture.h"


namespace svulkan {

VulkanTextureData::VulkanTextureData(vk::PhysicalDevice physicalDevice, vk::Device device,
                                     const vk::Extent2D &extent, vk::ImageTiling tiling,
                                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memoryProperties,
                                     bool anisotropyEnable)

    : mFormat(vk::Format::eR8G8B8A8Unorm), mExtent(extent) {
  mImageData = std::make_unique<VulkanImageData>(physicalDevice, device, mFormat, extent, 1, tiling, usage,
                                                 vk::ImageLayout::eUndefined, memoryProperties,
                                                 vk::ImageAspectFlagBits::eColor);

  mTextureSampler = device.createSamplerUnique(vk::SamplerCreateInfo(
      {}, vk::Filter::eLinear, vk::Filter::eLinear,
      vk::SamplerMipmapMode::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
      vk::SamplerAddressMode::eRepeat, 0.f, anisotropyEnable, anisotropyEnable ? 16.f : 0.f,
      false, vk::CompareOp::eNever, 0.f, 0.f, vk::BorderColor::eFloatOpaqueBlack));

}
}
