#pragma once
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "common/log.h"
#include "imgui_util.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "GLFW/glfw3.h"

namespace svulkan
{

struct VulkanFrame {
  // vk::UniqueCommandBuffer mCommandBuffer;
  // vk::UniqueFence mFence;
  
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

static void checkVKResult(VkResult err) {
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
  void newFrame() {
    glfwPollEvents();
    mSemaphoreIndex = (mSemaphoreIndex + 1) % mFrameSemaphores.size();
    auto result = mDevice.acquireNextImageKHR(mSwapchain.get(), UINT64_MAX,
                                              mFrameSemaphores[mSemaphoreIndex].mImageAcquiredSemaphore.get(),
                                              {});
    if (result.result != vk::Result::eSuccess) {
      log::error("Acquire image failed");
    }
    mFrameIndex = result.value;

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A) + 'q' - 'a')) {
      log::info("q pressed");
    }

    auto mousePos = ImGui::GetMousePos();

    static bool firstFrame = true;
    if (firstFrame) {
      firstFrame = false;
    } else {
      mMouseDelta = {mousePos.x - mMousePos.x, mousePos.y - mMousePos.y};
    }
    mMousePos = mousePos;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();
  }

  bool isKeyDown(char key) {
    return ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A) + key - 'a');
  }
  
  ImVec2 getMouseDelta() {
    return mMouseDelta;
  }

  bool isMouseKeyDown(int key) {
    return ImGui::IsMouseDown(key);
  }

  bool presentFrameWithImgui(vk::Queue graphicsQueue, vk::Queue presentQueue,
                             vk::Semaphore renderCompleteSemaphore, vk::Fence frameCompleteFence) {
    vk::ClearValue clearValue{};
    mDevice.resetCommandPool(mFrames[mFrameIndex].mImguiCommandPool.get(), {});
    mFrames[mFrameIndex].mImguiCommandBuffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::RenderPassBeginInfo info (mImguiRenderPass.get(), mFrames[mFrameIndex].mImguiFramebuffer.get(),
                                  {{0,0},{mWidth, mHeight}}, 1, &clearValue);
    mFrames[mFrameIndex].mImguiCommandBuffer->beginRenderPass(info, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), mFrames[mFrameIndex].mImguiCommandBuffer.get());
    mFrames[mFrameIndex].mImguiCommandBuffer->endRenderPass();
    mFrames[mFrameIndex].mImguiCommandBuffer->end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo{1, &renderCompleteSemaphore, &waitStage,
      1, &mFrames[mFrameIndex].mImguiCommandBuffer.get(),
      1, &mFrameSemaphores[mSemaphoreIndex].mImguiCompleteSemaphore.get()};
    graphicsQueue.submit(submitInfo, frameCompleteFence);

    return presentQueue.presentKHR({1, &mFrameSemaphores[mSemaphoreIndex].mImguiCompleteSemaphore.get(),
        1, &mSwapchain.get(), &mFrameIndex}) == vk::Result::eSuccess;
  }

  void initImgui(GLFWwindow *window, vk::Instance instance,
                 uint32_t graphicsQueueFamilyIndex, vk::Queue graphicsQueue,
                 vk::DescriptorPool descriptorPool, vk::CommandPool commandPool) {
    mImguiRenderPass = createImguiRenderPass(mDevice, mSurfaceFormat.format);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = mPhysicalDevice;
    initInfo.Device = mDevice;
    initInfo.QueueFamily = graphicsQueueFamilyIndex;
    initInfo.Queue = graphicsQueue;

    initInfo.PipelineCache = {};
    initInfo.DescriptorPool = descriptorPool;
    initInfo.Allocator = {};
    initInfo.MinImageCount = mMinImageCount;
    initInfo.ImageCount = mMinImageCount;
    initInfo.CheckVkResultFn = checkVKResult;
    ImGui_ImplVulkan_Init(&initInfo, mImguiRenderPass.get());

    OneTimeSubmit(mDevice, commandPool, graphicsQueue,
                  [](vk::CommandBuffer cb) { ImGui_ImplVulkan_CreateFontsTexture(cb); });
    log::info("Imgui initialized");
  }

  inline vk::Semaphore getImageAcquiredSemaphore() {
    return mFrameSemaphores[mSemaphoreIndex].mImageAcquiredSemaphore.get(); }

  inline vk::Image getBackBuffer() const { return mFrames[mFrameIndex].mBackbuffer; }


  VulkanWindow(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, 
               std::vector<vk::Format> const &requestFormats,
               vk::ColorSpaceKHR requestColorSpace,
               std::vector<vk::PresentModeKHR> const& requestModes={vk::PresentModeKHR::eFifo},
               uint32_t minImageCount=3)
      : mDevice(device), mPhysicalDevice(physicalDevice), mSurface(surface), mMinImageCount(minImageCount) {
    selectSurfaceFormat(requestFormats, requestColorSpace);
    selectPresentMode(requestModes);
  }

  void selectSurfaceFormat(std::vector<vk::Format> const &requestFormats,
                           vk::ColorSpaceKHR requestColorSpace) {
    assert(requestFormats.size() > 0);

    // Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at
    // image creation Assuming that the default behavior is without setting this bit, there is no need for
    // separate Swapchain image and image view format Additionally several new color spaces were introduced with
    // Vulkan Spec v1.0.40, hence we must make sure that a format with the mostly available color space,
    // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used.
    auto avail_formats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface);
    if (avail_formats.size() == 0) {
      throw std::runtime_error("No surface format is available");
    }

    if (avail_formats.size() == 1) {
      if (avail_formats[0].format == vk::Format::eUndefined) {
        vk::SurfaceFormatKHR ret;
        ret.format = requestFormats[0];
        ret.colorSpace = requestColorSpace;
        mSurfaceFormat = ret;
        return;
      } 
      // No point in searching another format
      mSurfaceFormat = avail_formats[0];
      return;
    }

    // Request several formats, the first found will be used
    for (uint32_t request_i = 0; request_i < requestFormats.size(); request_i++) {
      for (uint32_t avail_i = 0; avail_i < avail_formats.size(); avail_i++) {
        if (avail_formats[avail_i].format == requestFormats[request_i] &&
            avail_formats[avail_i].colorSpace == requestColorSpace) {
          mSurfaceFormat = avail_formats[avail_i];
          return;
        }
      }
    }

    log::warn("None of the requested surface formats is available");
    // If none of the requested image formats could be found, use the first available
    mSurfaceFormat = avail_formats[0];
  }

  void selectPresentMode(std::vector<vk::PresentModeKHR> const &requestModes) {
    assert(requestModes.size() > 0);

    // Request a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is
    // mandatory
    auto avail_modes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface);

    for (uint32_t request_i = 0; request_i < requestModes.size(); request_i++) {
      for (uint32_t avail_i = 0; avail_i < avail_modes.size(); avail_i++) {
        if (requestModes[request_i] == avail_modes[avail_i]) {
          mPresentMode = requestModes[request_i];
          return;
        }
      }
    }

    mPresentMode = vk::PresentModeKHR::eFifo; // always available
  }

  void recreateSwapchain(uint32_t w, uint32_t h) {
    if (mMinImageCount == 0) {
      throw std::runtime_error("Invalid min image count specified");
    }
    
    vk::SwapchainCreateInfoKHR info({}, mSurface, mMinImageCount,
                                    mSurfaceFormat.format, mSurfaceFormat.colorSpace,
                                    {w, h}, 1,
                                    vk::ImageUsageFlagBits::eColorAttachment |
                                    vk::ImageUsageFlagBits::eTransferDst,
                                    vk::SharingMode::eExclusive, 0, nullptr,
                                    vk::SurfaceTransformFlagBitsKHR::eIdentity,
                                    vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                    mPresentMode, VK_TRUE, mSwapchain.get());
    auto cap = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface);
    if (info.minImageCount < cap.minImageCount) {
      info.minImageCount = cap.minImageCount;
    } else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount) {
      info.minImageCount = cap.maxImageCount;
    }
    if (cap.currentExtent.width != 0xffffffff) {
      mWidth = info.imageExtent.width = cap.currentExtent.width;
      mHeight = info.imageExtent.height = cap.currentExtent.height;
    } else {
      mWidth = w;
      mWidth = h;
    }
    mSwapchain = mDevice.createSwapchainKHRUnique(info);
    auto images = mDevice.getSwapchainImagesKHR(mSwapchain.get());

    assert(images.size() >= mMinImageCount);
    assert(images.size() < 16);

    mFrames.resize(images.size());
    mFrameSemaphores.resize(images.size());
    for (uint32_t i = 0; i < images.size(); ++i) {
      mFrameSemaphores[i].mImageAcquiredSemaphore = mDevice.createSemaphoreUnique({});
      mFrames[i].mBackbuffer = images[i];
      vk::ImageViewCreateInfo info{{}, mFrames[i].mBackbuffer, vk::ImageViewType::e2D, mSurfaceFormat.format,
                                   {vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                    vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
                                   {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
      mFrames[i].mBackbufferView = mDevice.createImageViewUnique(info);
    }
  }

  void recreateImguiResources(uint32_t queueFamily) {
    for (uint32_t i = 0; i < mFrames.size(); ++i) {
      mFrames[i].mImguiCommandBuffer.reset();
      mFrames[i].mImguiCommandPool.reset();
    }
    mDevice.createCommandPoolUnique({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamily});
    for (uint32_t i = 0; i < mFrames.size(); ++i) {
      mFrames[i].mImguiCommandPool = mDevice.createCommandPoolUnique(
          vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamily));
      mFrames[i].mImguiCommandBuffer = std::move(mDevice.allocateCommandBuffersUnique(
          {mFrames[i].mImguiCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1}).front());


      vk::FramebufferCreateInfo info ({}, mImguiRenderPass.get(), 1, &mFrames[i].mBackbufferView.get(),
                                      mWidth, mHeight, 1);
      mFrames[i].mImguiFramebuffer = mDevice.createFramebufferUnique(info);
    }
    for (uint32_t i = 0; i < mFrameSemaphores.size(); ++i) {
      mFrameSemaphores[i].mImguiCompleteSemaphore = mDevice.createSemaphoreUnique({});
    }
  }
};
}

