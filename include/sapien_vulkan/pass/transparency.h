#pragma once
#include "sapien_vulkan/internal/vulkan.h"

namespace svulkan {
class VulkanContext;

class TransparencyPass {
  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;
  vk::UniquePipeline mPipeline;
  vk::UniqueFramebuffer mFramebuffer;

public:
  TransparencyPass(VulkanContext &context);

  TransparencyPass(TransparencyPass const &other) = delete;
  TransparencyPass &operator=(TransparencyPass const &other) = delete;

  TransparencyPass(TransparencyPass &&other) = default;
  TransparencyPass &operator=(TransparencyPass &&other) = default;

  void initializePipeline(std::string const &shaderDir,
                          std::vector<vk::DescriptorSetLayout> const &layouts,
                          std::vector<vk::Format> const &colorFormats, vk::Format depthFormat,
                          vk::CullModeFlags cullMode, vk::FrontFace frontFace);
  void initializeFramebuffer(std::vector<vk::ImageView> const &colorImageViews,
                             vk::ImageView depthImageView, vk::Extent2D const &extent);

  inline vk::Framebuffer getFramebuffer() { return mFramebuffer.get(); }
  inline vk::RenderPass getRenderPass() { return mRenderPass.get(); }
  inline vk::PipelineLayout getPipelineLayout() { return mPipelineLayout.get(); }
  inline vk::Pipeline getPipeline() { return mPipeline.get(); }
};

} // namespace svulkan
