#pragma once
#ifdef ON_SCREEN
#include "sapien_vulkan/internal/vulkan.h"

namespace svulkan {

vk::UniqueRenderPass createImguiRenderPass(vk::Device device, vk::Format format) {
  
  vk::AttachmentDescription attachment{{}, format,
    vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR};
  vk::AttachmentReference colorAttachment{0, vk::ImageLayout::eColorAttachmentOptimal};

  vk::SubpassDescription subpass ({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachment);
  vk::SubpassDependency dependency {VK_SUBPASS_EXTERNAL, 0,
    vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eMemoryWrite, vk::AccessFlagBits::eColorAttachmentWrite};

  vk::RenderPassCreateInfo info({}, 1, &attachment, 1, &subpass, 1, &dependency);
  return device.createRenderPassUnique(info);
}

vk::UniqueFramebuffer createImguiFramebuffer(vk::Device device,
                                             vk::RenderPass renderPass,
                                             vk::ImageView view,
                                             uint32_t width, uint32_t height) {
  vk::FramebufferCreateInfo info({}, renderPass, 1, &view, width, height, 1);
  return device.createFramebufferUnique(info);
}

}
#endif
