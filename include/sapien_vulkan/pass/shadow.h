#pragma once
#include "sapien_vulkan/internal/vulkan.h"

namespace svulkan {
class VulkanContext;

class ShadowPass {
  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;
  vk::UniquePipeline mPipeline;
  vk::UniqueFramebuffer mFramebuffer;

public:
  ShadowPass(VulkanContext &context);

  ShadowPass(ShadowPass const &other) = delete;
  ShadowPass &operator=(ShadowPass const &other) = delete;

  ShadowPass(ShadowPass &&other) = default;
  ShadowPass &operator=(ShadowPass &&other) = default;

  void initializePipeline(std::string const &shaderDir,
                          std::vector<vk::DescriptorSetLayout> const &layouts,
                          vk::Format depthFormat, vk::CullModeFlags cullMode, vk::FrontFace frontFace);
  void initializeFramebuffer(vk::ImageView depthImageView, vk::Extent2D const &extent);

  inline vk::Framebuffer getFramebuffer() { return mFramebuffer.get(); }
  inline vk::RenderPass getRenderPass() { return mRenderPass.get(); }
  inline vk::PipelineLayout getPipelineLayout() { return mPipelineLayout.get(); }
  inline vk::Pipeline getPipeline() { return mPipeline.get(); }
};

} // namespace svulkan
