#pragma once
#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

struct ImGuiFrame {
  vk::UniqueCommandPool mCommandPool;
  vk::UniqueCommandBuffer mCommandBuffer;
  vk::UniqueFence mFence;

  vk::Image mBackbuffer; // image from swapchain
  vk::UniqueImageView mBackbufferView;
  vk::UniqueFramebuffer mFramebuffer;
};

struct ImGuiFrameSemaphores {
  vk::UniqueSemaphore ImageAcquiredSemaphore;
  vk::UniqueSemaphore RenderCompleteSemaphore;
};

struct ImGuiVulkanWindow {
  int mWidth;
  int mHeight;
  vk::SurfaceKHR mSurface;
  vk::SurfaceFormatKHR mSurfaceFormat;
  vk::PresentModeKHR mPresentMode;
  uint32_t mFrameIndex;
  uint32_t mSemaphoreIndex;

  std::vector<ImGuiFrame> mFrames;
  std::vector<ImGuiFrameSemaphores> mFrameSemaphores;

  vk::UniqueRenderPass mRenderPass;
  vk::UniqueSwapchainKHR mSwapchain;

  ImGuiVulkanWindow() {
    mWidth = 0;
    mHeight = 0;
    mFrameIndex = 0;
    mSemaphoreIndex = 0;
  }
};

vk::SurfaceFormatKHR SelectSurfaceFormat(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
                                         std::vector<vk::Format> const &request_formats,
                                         vk::ColorSpaceKHR request_color_space) {
  assert(request_formats.size() > 0);

  // Per Spec Format and View Format are expected to be the same unless VK_IMAGE_CREATE_MUTABLE_BIT was set at
  // image creation Assuming that the default behavior is without setting this bit, there is no need for
  // separate Swapchain image and image view format Additionally several new color spaces were introduced with
  // Vulkan Spec v1.0.40, hence we must make sure that a format with the mostly available color space,
  // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, is found and used.
  auto avail_formats = physical_device.getSurfaceFormatsKHR(surface);
  if (avail_formats.size() == 1) {
    if (avail_formats[0].format == vk::Format::eUndefined) {
      vk::SurfaceFormatKHR ret;
      ret.format = request_formats[0];
      ret.colorSpace = request_color_space;
      return ret;
    } else {
      // No point in searching another format
      return avail_formats[0];
    }
  } else {
    // Request several formats, the first found will be used
    for (int request_i = 0; request_i < request_formats.size(); request_i++)
      for (uint32_t avail_i = 0; avail_i < avail_formats.size(); avail_i++)
        if (avail_formats[avail_i].format == request_formats[request_i] &&
            avail_formats[avail_i].colorSpace == request_color_space)
          return avail_formats[avail_i];

    // If none of the requested image formats could be found, use the first available
    return avail_formats[0];
  }
}

vk::PresentModeKHR SelectPresentMode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
                                     std::vector<vk::PresentModeKHR> const &request_modes) {
  assert(request_modes.size() > 0);

  // Request a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is
  // mandatory
  auto avail_modes = physical_device.getSurfacePresentModesKHR(surface);

  for (int request_i = 0; request_i < request_modes.size(); request_i++) {
    for (uint32_t avail_i = 0; avail_i < avail_modes.size(); avail_i++) {
      if (request_modes[request_i] == avail_modes[avail_i]) {
        return request_modes[request_i];
      }
    }
  }

  return vk::PresentModeKHR::eFifo; // always available
}

void CreateWindowSwapChain(vk::PhysicalDevice physical_device, vk::Device device, ImGuiVulkanWindow *wd,
                           const vk::AllocationCallbacks *allocator, int w, int h, uint32_t min_image_count) {
  vk::UniqueSwapchainKHR old_swapchain = std::move(wd->mSwapchain);
  device.waitIdle();

  if (min_image_count == 0) {
    throw std::runtime_error("Invalid min image count specified");
  }

  // Create Swapchain
  {
    vk::SwapchainCreateInfoKHR info;
    info.surface = wd->mSurface;
    info.minImageCount = min_image_count;
    info.imageFormat = wd->mSurfaceFormat.format;
    info.imageColorSpace = wd->mSurfaceFormat.colorSpace;
    info.imageArrayLayers = 1;
    info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    info.imageSharingMode = vk::SharingMode::eExclusive;

    info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    info.presentMode = wd->mPresentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = *old_swapchain;

    auto cap = physical_device.getSurfaceCapabilitiesKHR(wd->mSurface);
    if (info.minImageCount < cap.minImageCount) {
      info.minImageCount = cap.minImageCount;
    } else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount) {
      info.minImageCount = cap.maxImageCount;
    }
    if (cap.currentExtent.width == 0xffffffff) {
      info.imageExtent.width = wd->mWidth = w;
      info.imageExtent.height = wd->mHeight = h;
    } else {
      info.imageExtent.width = wd->mWidth = cap.currentExtent.width;
      info.imageExtent.height = wd->mHeight = cap.currentExtent.height;
    }

    wd->mSwapchain = device.createSwapchainKHRUnique(info, allocator);
    auto images = device.getSwapchainImagesKHR(*wd->mSwapchain);

    assert(images.size() >= min_image_count);
    assert(images.size() < 16);

    wd->mFrames.resize(images.size());
    wd->mFrameSemaphores.resize(images.size());

    for (uint32_t i = 0; i < images.size(); ++i) {
      wd->mFrames[i].mBackbuffer = images[i];
    }
  }

  // Create the Render Pass
  {
    vk::AttachmentDescription attachment;
    attachment.format = wd->mSurfaceFormat.format;
    attachment.samples = vk::SampleCountFlagBits::e1;
    attachment.loadOp = vk::AttachmentLoadOp::eLoad;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;
    attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment.initialLayout = vk::ImageLayout::eUndefined;
    attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eAllCommands;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo info;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    wd->mRenderPass = device.createRenderPassUnique(info);
  }

  // Create The Image Views
  {
    vk::ImageViewCreateInfo info;
    info.viewType = vk::ImageViewType::e2D;
    info.format = wd->mSurfaceFormat.format;
    info.components.r = vk::ComponentSwizzle::eR;
    info.components.g = vk::ComponentSwizzle::eG;
    info.components.b = vk::ComponentSwizzle::eB;
    info.components.a = vk::ComponentSwizzle::eA;
    vk::ImageSubresourceRange image_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    info.subresourceRange = image_range;

    for (uint32_t i = 0; i < wd->mFrames.size(); i++) {
      info.image = wd->mFrames[i].mBackbuffer;
      wd->mFrames[i].mBackbufferView = device.createImageViewUnique(info, allocator);
    }
  }

  // Create Framebuffer
  {
    vk::ImageView attachment[1];
    vk::FramebufferCreateInfo info;
    info.renderPass = *wd->mRenderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = wd->mWidth;
    info.height = wd->mHeight;
    info.layers = 1;

    for (uint32_t i = 0; i < wd->mFrames.size(); i++) {
      attachment[0] = *wd->mFrames[i].mBackbufferView;
      wd->mFrames[i].mFramebuffer = device.createFramebufferUnique(info, allocator);
    }
  }
}

void CreateWindowCommandBuffers(vk::PhysicalDevice physical_device, vk::Device device, ImGuiVulkanWindow *wd,
                                uint32_t queue_family, const vk::AllocationCallbacks *allocator) {
  // Create Command Buffers
  for (uint32_t i = 0; i < wd->mFrames.size(); i++) {
    ImGuiFrame *fd = &wd->mFrames[i];
    ImGuiFrameSemaphores *fsd = &wd->mFrameSemaphores[i];

    // free command buffer before command pool
    fd->mCommandBuffer.reset();
    fd->mCommandPool.reset();

    fd->mCommandPool = device.createCommandPoolUnique(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queue_family),
        allocator);
    fd->mCommandBuffer = std::move(device
                                       .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
                                           *fd->mCommandPool, vk::CommandBufferLevel::ePrimary, 1))
                                       .front());
    fd->mFence = device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled), allocator);
    fsd->ImageAcquiredSemaphore = device.createSemaphoreUnique(vk::SemaphoreCreateInfo(), allocator);
    fsd->RenderCompleteSemaphore = device.createSemaphoreUnique(vk::SemaphoreCreateInfo(), allocator);
  }
}

void CreateImGuiVulkanWindow(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device,
                       ImGuiVulkanWindow *wd, uint32_t queue_family, const vk::AllocationCallbacks *allocator,
                       int width, int height, uint32_t min_image_count) {
  (void)instance;
  CreateWindowSwapChain(physical_device, device, wd, allocator, width, height, min_image_count);
  CreateWindowCommandBuffers(physical_device, device, wd, queue_family, allocator);
}
