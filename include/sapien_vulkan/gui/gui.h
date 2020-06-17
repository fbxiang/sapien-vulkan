#pragma once
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
  vk::Device mDevice;
  vk::PhysicalDevice mPhysicalDevice;
  vk::SurfaceKHR mSurface;
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

 public:
  inline vk::SwapchainKHR getSwapchain() const { return mSwapchain.get(); }
  inline vk::Format getBackBufferFormat() const { return mSurfaceFormat.format; }
  inline uint32_t getWidth() const { return mWidth; }
  inline uint32_t getHeight() const { return mHeight; }
  inline uint32_t getFrameIndex() const { return mFrameIndex; }

 public:
  void newFrame();

  bool isKeyDown(char key); 
  
  ImVec2 getMouseDelta(); 

  bool isMouseKeyDown(int key); 

  bool presentFrameWithImgui(vk::Queue graphicsQueue, vk::Queue presentQueue,
                             vk::Semaphore renderCompleteSemaphore, vk::Fence frameCompleteFence); 

  void initImgui(GLFWwindow *window, vk::Instance instance,
                 uint32_t graphicsQueueFamilyIndex, vk::Queue graphicsQueue,
                 vk::DescriptorPool descriptorPool, vk::CommandPool commandPool); 

  inline vk::Semaphore getImageAcquiredSemaphore() {
    return mFrameSemaphores[mSemaphoreIndex].mImageAcquiredSemaphore.get(); }

  inline vk::Image getBackBuffer() const { return mFrames[mFrameIndex].mBackbuffer; }


  VulkanWindow(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, 
               std::vector<vk::Format> const &requestFormats,
               vk::ColorSpaceKHR requestColorSpace,
               std::vector<vk::PresentModeKHR> const& requestModes={vk::PresentModeKHR::eFifo},
               uint32_t minImageCount=3);

  /** Called when the window is resized to recreate the sawpchain */
  void recreateSwapchain(uint32_t w, uint32_t h); 

  /** Called after swapchain recreation to update ImGui related resources */
  void recreateImguiResources(uint32_t queueFamily); 

 private:
  /** Called at initialization time  */
  void selectSurfaceFormat(std::vector<vk::Format> const &requestFormats,
                           vk::ColorSpaceKHR requestColorSpace);
  /** Called at initialization time  */
  void selectPresentMode(std::vector<vk::PresentModeKHR> const &requestModes); 
};
}

