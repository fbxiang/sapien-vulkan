#include "sapien_vulkan/gui/gui.h"
#include "sapien_vulkan/gui/imgui_util.hpp"

namespace svulkan
{

VulkanWindow::VulkanWindow(vk::Instance instance, vk::Device device, vk::PhysicalDevice physicalDevice,
                           uint32_t graphicsQueueFamilyIndex,
                           std::vector<vk::Format> const &requestFormats, vk::ColorSpaceKHR requestColorSpace,
                           uint32_t width, uint32_t height,
                           std::vector<vk::PresentModeKHR> const& requestModes, uint32_t minImageCount)
    : mInstance(instance), mDevice(device), mPhysicalDevice(physicalDevice), mMinImageCount(minImageCount) {
  createGlfwWindow(instance, graphicsQueueFamilyIndex, width, height);
  selectSurfaceFormat(requestFormats, requestColorSpace);
  selectPresentMode(requestModes);
}

void VulkanWindow::newFrame() {
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
}

bool VulkanWindow::isKeyDown(char key) {
  return ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A) + key - 'a');
}

ImVec2 VulkanWindow::getMouseDelta() {
  mMouseDelta.x = std::clamp(mMouseDelta.x, -100.f, 100.f);
  mMouseDelta.y = std::clamp(mMouseDelta.y, -100.f, 100.f);
  return mMouseDelta;
}

ImVec2 VulkanWindow::getMousePosition() {
  return mMousePos;
}

bool VulkanWindow::isMouseKeyDown(int key) {
  return ImGui::IsMouseDown(key);
}

bool VulkanWindow::presentFrameWithImgui(vk::Queue graphicsQueue, vk::Queue presentQueue,
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

void VulkanWindow::initImgui(vk::DescriptorPool descriptorPool, vk::CommandPool commandPool) {
  mImguiRenderPass = createImguiRenderPass(mDevice, mSurfaceFormat.format);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(mWindow, true);
  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = mInstance;
  initInfo.PhysicalDevice = mPhysicalDevice;
  initInfo.Device = mDevice;
  initInfo.QueueFamily = mPresentQueueFamilyIndex;
  initInfo.Queue = getGraphicsQueue();

  initInfo.PipelineCache = {};
  initInfo.DescriptorPool = descriptorPool;
  initInfo.Allocator = {};
  initInfo.MinImageCount = mMinImageCount;
  initInfo.ImageCount = mMinImageCount;
  initInfo.CheckVkResultFn = checkVKResult;
  ImGui_ImplVulkan_Init(&initInfo, mImguiRenderPass.get());

  OneTimeSubmit(mDevice, commandPool, getGraphicsQueue(),
                [](vk::CommandBuffer cb) { ImGui_ImplVulkan_CreateFontsTexture(cb); });
  log::info("Imgui initialized");
}

void VulkanWindow::selectSurfaceFormat(std::vector<vk::Format> const &requestFormats,
                                          vk::ColorSpaceKHR requestColorSpace) {
  assert(requestFormats.size() > 0);

  // Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at
  // image creation Assuming that the default behavior is without setting this bit, there is no need for
  // separate Swapchain image and image view format Additionally several new color spaces were introduced with
  // Vulkan Spec v1.0.40, hence we must make sure that a format with the mostly available color space,
  // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used.
  auto avail_formats = mPhysicalDevice.getSurfaceFormatsKHR(mSurface.get());
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

void VulkanWindow::selectPresentMode(std::vector<vk::PresentModeKHR> const &requestModes) {
  assert(requestModes.size() > 0);

  // Request a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is
  // mandatory
  auto avail_modes = mPhysicalDevice.getSurfacePresentModesKHR(mSurface.get());

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

void VulkanWindow::updateSize(uint32_t w, uint32_t h) {
  recreateSwapchain(w, h);
  recreateImguiResources();
  mDevice.waitIdle();
}

void VulkanWindow::recreateSwapchain(uint32_t w, uint32_t h) {
  if (mMinImageCount == 0) {
    throw std::runtime_error("Invalid min image count specified");
  }
    
  vk::SwapchainCreateInfoKHR info({}, mSurface.get(), mMinImageCount,
                                  mSurfaceFormat.format, mSurfaceFormat.colorSpace,
                                  {w, h}, 1,
                                  vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eTransferDst,
                                  vk::SharingMode::eExclusive, 0, nullptr,
                                  vk::SurfaceTransformFlagBitsKHR::eIdentity,
                                  vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                  mPresentMode, VK_TRUE, mSwapchain.get());
  auto cap = mPhysicalDevice.getSurfaceCapabilitiesKHR(mSurface.get());
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

void VulkanWindow::recreateImguiResources() {
  for (uint32_t i = 0; i < mFrames.size(); ++i) {
    mFrames[i].mImguiCommandBuffer.reset();
    mFrames[i].mImguiCommandPool.reset();
  }
  for (uint32_t i = 0; i < mFrames.size(); ++i) {
    mFrames[i].mImguiCommandPool = mDevice.createCommandPoolUnique(
        {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, mGraphicsQueueFamilyIndex});
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

void VulkanWindow::createGlfwWindow(vk::Instance instance, uint32_t graphicsQueueFamilyIndex,
                                    uint32_t width, uint32_t height) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  mWindow = glfwCreateWindow(width, height, "vulkan", nullptr, nullptr);

  VkSurfaceKHR tmpSurface;

  auto result = glfwCreateWindowSurface(instance, mWindow, nullptr, &tmpSurface);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("create window failed: glfwCreateWindowSurface failed");
  }
  mSurface = vk::UniqueSurfaceKHR(tmpSurface, instance);

  if (!mPhysicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, mSurface.get())) {
    throw std::runtime_error("create window failed: graphics device does not have present capability");
  }

  mGraphicsQueueFamilyIndex = mPresentQueueFamilyIndex = graphicsQueueFamilyIndex;
}

void VulkanWindow::close() {
  mClosed = true;
  glfwSetWindowShouldClose(mWindow, true);

  mDevice.waitIdle();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();

  mImguiRenderPass.reset();
  mFrameSemaphores.clear();
  mFrames.clear();
  mSwapchain.reset();
  mSurface.reset();
}

bool VulkanWindow::isClosed() {
  return mClosed;
}

VulkanWindow::~VulkanWindow() {
  glfwDestroyWindow(mWindow);
}

}
