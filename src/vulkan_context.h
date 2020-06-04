#pragma once

#ifdef ON_SCREEN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

namespace svulkan
{

class VulkanContext {

 private:
  vk::PhysicalDevice mPhysicalDevice;
  vk::UniqueInstance mInstance;
  vk::UniqueDevice mDevice;
  vk::UniqueCommandPool mCommandPool;

  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  vk::UniqueSurfaceKHR mSurface;
  GLFWwindow *mWindow;
#ifdef ON_SCREEN

#endif

 public:
  VulkanContext();

  /** Get the graphics queue */
  vk::Queue getGraphicsQueue() const;

#ifdef ON_SCREEN
  vk::Queue getPresentQueue() const;
#endif

  inline vk::Device getDevice() const { return mDevice.get(); }

 private:
#ifdef VK_VALIDATION
  bool checkValidationLayerSupport();
#endif

#ifdef ON_SCREEN
  void createWindow();
#endif

  /** Create Vulkan instance */
  void createInstance();

  /** Choose physical device */
  void pickPhysicalDevice();

  /** Create logical device */
  void createLogicalDevice();

  /** Create command pool for rendering */
  void createCommandPool();


};

}
