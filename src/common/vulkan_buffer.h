#pragma once
#include "vulkan_util.h"

namespace svulkan {

struct VulkanBufferData {
  vk::UniqueBuffer mBuffer;
  vk::UniqueDeviceMemory mMemory;

  VulkanBufferData(vk::PhysicalDevice physicalDevice, vk::Device device, vk::DeviceSize size,
                   vk::BufferUsageFlags usage,
                   vk::MemoryPropertyFlags propertyFlags = vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent);


  /** Upload data to Vulkan buffer */
  template <typename DataType> void upload(vk::Device device, DataType const &data) const {
#if !defined(NDEBUG)
    assert((m_propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent) &&
           (m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible));
    assert(sizeof(DataType) <= m_size);
#endif
    copyToDevice(device, mMemory.get(), data);
  }


  /** Upload data vector to Vulkan buffer */
  template <typename DataType>
  void upload(vk::Device device, std::vector<DataType> const &data, size_t stride = 0) const {
#if !defined(NDEBUG)
    assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible);
#endif

    size_t elementSize = stride ? stride : sizeof(DataType);
#if !defined(NDEBUG)
    assert(sizeof(DataType) <= elementSize);
#endif
    copyToDevice(device, mMemory.get(), data.data(), data.size(), elementSize);
  }

  /** Upload with staging buffer */
  template <typename DataType>
  void uploadNoWait(vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool,
              vk::Queue queue, std::vector<DataType> const &data, size_t stride) const {
#if !defined(NDEBUG)
    assert(m_usage & vk::BufferUsageFlagBits::eTransferDst);
    assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);
#endif
    size_t elementSize = stride ? stride : sizeof(DataType);
#if !defined(NDEBUG)
    assert(sizeof(DataType) <= elementSize);
#endif

    size_t dataSize = data.size() * elementSize;
    VulkanBufferData stagingBuffer(physicalDevice, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
    copyToDevice<DataType>(device, stagingBuffer.mMemory.get(), data.data(), data.size(), elementSize);

    OneTimeSubmitNoWait(device, commandPool, queue, [&](vk::CommandBuffer commandBuffer) {
      commandBuffer.copyBuffer(*stagingBuffer.mBuffer, *mBuffer, vk::BufferCopy(0, 0, dataSize));
    });
  }

  /** Upload with staging buffer */
  template <typename DataType>
  void upload(vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool,
              vk::Queue queue, std::vector<DataType> const &data, size_t stride) const {
#if !defined(NDEBUG)
    assert(m_usage & vk::BufferUsageFlagBits::eTransferDst);
    assert(m_propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal);
#endif
    size_t elementSize = stride ? stride : sizeof(DataType);
#if !defined(NDEBUG)
    assert(sizeof(DataType) <= elementSize);
#endif

    size_t dataSize = data.size() * elementSize;
    VulkanBufferData stagingBuffer(physicalDevice, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
    copyToDevice<DataType>(device, stagingBuffer.mMemory.get(), data.data(), data.size(), elementSize);

    oneTimeSubmit(device, commandPool, queue, [&](vk::CommandBuffer commandBuffer) {
      commandBuffer.copyBuffer(*stagingBuffer.mBuffer, *mBuffer, vk::BufferCopy(0, 0, dataSize));
    });
  }
  
#if !defined(NDEBUG)
 private:
  vk::DeviceSize m_size;
  vk::BufferUsageFlags m_usage;
  vk::MemoryPropertyFlags m_propertyFlags;
#endif

};
}
