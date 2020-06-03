#include "vulkan_buffer.h"

namespace svulkan 
{

VulkanBufferData::VulkanBufferData(vk::PhysicalDevice physicalDevice, vk::Device device, vk::DeviceSize size,
                 vk::BufferUsageFlags usage,
                 vk::MemoryPropertyFlags propertyFlags) {
  mBuffer = device.createBufferUnique(vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage));
  mMemory = allocateMemory(device, physicalDevice.getMemoryProperties(),
                           device.getBufferMemoryRequirements(mBuffer.get()), propertyFlags);
  device.bindBufferMemory(mBuffer.get(), mMemory.get(), 0);

#if !defined(NDEBUG)
  m_size = size;
  m_usage = usage;
  m_propertyFlags = propertyFlags;
#endif
}

}
