#pragma once
#ifdef ON_SCREEN
#define GLFW_INCLUDE_VULKAN

#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "sapien_vulkan/common/log.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "GLFW/glfw3.h"

namespace svulkan
{

struct VulkanFrame {
  vk::Image mBackbuffer;
  vk::UniqueImageView mBackbufferView;

  vk::UniqueFramebuffer mImguiFramebuffer;
  vk::UniqueCommandPool mImguiCommandPool;
  vk::UniqueCommandBuffer mImguiCommandBuffer;
};

struct VulkanFrameSemaphores {
  vk::UniqueSemaphore mImageAcquiredSemaphore;
  vk::UniqueSemaphore mImguiCompleteSemaphore;
};

static inline void checkVKResult(VkResult err) {
  if (err != 0) {
    log::error("Vulkan result check failed.");
  }
}

class VulkanWindow {
  GLFWwindow *mWindow;
  // vk::UniqueSurfaceKHR mSurface;
  vk::UniqueSurfaceKHR mSurface;
  uint32_t mGraphicsQueueFamilyIndex;
  uint32_t mPresentQueueFamilyIndex;

  vk::Instance mInstance;
  vk::Device mDevice;
  vk::PhysicalDevice mPhysicalDevice;
  uint32_t mMinImageCount;

  uint32_t mWidth {0};
  uint32_t mHeight {0};
  vk::SurfaceFormatKHR mSurfaceFormat {};
  vk::PresentModeKHR mPresentMode { vk::PresentModeKHR::eFifo };

  uint32_t mFrameIndex {0};
  uint32_t mSemaphoreIndex {0};

  std::vector<VulkanFrame> mFrames {};
  std::vector<VulkanFrameSemaphores> mFrameSemaphores {};

  vk::UniqueSwapchainKHR mSwapchain {};

  // imgui resources
  vk::UniqueRenderPass mImguiRenderPass;

  ImVec2 mMousePos{0,0};
  ImVec2 mMouseDelta{0,0};
  ImVec2 mMouseWheelDelta{0,0};

  bool mClosed = false;

 public:
  inline vk::SwapchainKHR getSwapchain() const { return mSwapchain.get(); }
  inline vk::Format getBackBufferFormat() const { return mSurfaceFormat.format; }
  inline uint32_t getWidth() const { return mWidth; }
  inline uint32_t getHeight() const { return mHeight; }
  inline uint32_t getFrameIndex() const { return mFrameIndex; }

 public:
  void newFrame();

  bool isKeyDown(char key); 

  bool isKeyPressed(char key); 
  
  ImVec2 getMouseDelta(); 

  ImVec2 getMouseWheelDelta(); 

  ImVec2 getMousePosition();

  bool isMouseKeyDown(int key); 

  bool presentFrameWithImgui(vk::Queue graphicsQueue, vk::Queue presentQueue,
                             vk::Semaphore renderCompleteSemaphore, vk::Fence frameCompleteFence); 

  void initImgui(vk::DescriptorPool descriptorPool, vk::CommandPool commandPool); 

  inline vk::Semaphore getImageAcquiredSemaphore() {
    return mFrameSemaphores[mSemaphoreIndex].mImageAcquiredSemaphore.get(); }

  inline vk::Image getBackBuffer() const { return mFrames[mFrameIndex].mBackbuffer; }


  VulkanWindow(vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice,
               uint32_t graphicsQueueFamilyIndex,
               std::vector<vk::Format> const &requestFormats,
               vk::ColorSpaceKHR requestColorSpace, uint32_t width = 800, uint32_t height = 600,
               std::vector<vk::PresentModeKHR> const& requestModes={vk::PresentModeKHR::eFifo},
               uint32_t minImageCount=3);

  VulkanWindow(VulkanWindow const &other) = delete;
  VulkanWindow &operator=(VulkanWindow const &other) = delete;

  VulkanWindow(VulkanWindow &&other) = default;
  VulkanWindow &operator=(VulkanWindow &&other) = default;

  ~VulkanWindow();

  /** recreate the swapchain and ImGui resources with a new size */
  void updateSize(uint32_t w, uint32_t h);

  inline GLFWwindow* getWindow() const {return mWindow;}

  inline vk::Queue getPresentQueue() const { return mDevice.getQueue(mPresentQueueFamilyIndex, 0); }
  inline vk::Queue getGraphicsQueue() const { return mDevice.getQueue(mGraphicsQueueFamilyIndex, 0); }

  void close();
  bool isClosed();

 private:
  /** Called at initialization time  */
  void selectSurfaceFormat(std::vector<vk::Format> const &requestFormats,
                           vk::ColorSpaceKHR requestColorSpace);
  /** Called at initialization time  */
  void selectPresentMode(std::vector<vk::PresentModeKHR> const &requestModes); 

  void createGlfwWindow(vk::Instance instance, uint32_t graphicsQueueFamilyIndex,
                        uint32_t width, uint32_t height);

  /** Called when the window is resized to recreate the sawpchain */
  void recreateSwapchain(uint32_t w, uint32_t h); 

  /** Called after swapchain recreation to update ImGui related resources */
  void recreateImguiResources(); 
};

}

#endif
