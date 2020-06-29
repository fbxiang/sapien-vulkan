#pragma once
#include "sapien_vulkan/internal/vulkan.h"

namespace svulkan {
class VulkanContext;

class AxisPass {
  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;
  vk::UniquePipeline mPipeline;
  vk::UniqueFramebuffer mFramebuffer;

public:
  AxisPass(VulkanContext &context);
  AxisPass(AxisPass const &other) = delete;
  AxisPass &operator=(AxisPass const &other) = delete;

  AxisPass(AxisPass &&other) = default;
  AxisPass &operator=(AxisPass &&other) = default;

  void initializePipeline(std::string const &shaderDir,
                          std::vector<vk::DescriptorSetLayout> const &layouts,
                          std::vector<vk::Format> const &colorFormats, vk::Format depthFormat,
                          vk::CullModeFlags cullMode, vk::FrontFace frontFace, uint32_t maxNumAxes);
  void initializeFramebuffer(std::vector<vk::ImageView> const &colorImageViews,
                             vk::ImageView depthImageView, vk::Extent2D const &extent);

  inline vk::Framebuffer getFramebuffer() { return mFramebuffer.get(); }
  inline vk::RenderPass getRenderPass() { return mRenderPass.get(); }
  inline vk::PipelineLayout getPipelineLayout() { return mPipelineLayout.get(); }
  inline vk::Pipeline getPipeline() { return mPipeline.get(); }
};
} // namespace svulkan
