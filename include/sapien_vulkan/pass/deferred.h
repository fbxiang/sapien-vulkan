#pragma once
#include "sapien_vulkan/internal/vulkan.h"

namespace svulkan
{
class VulkanContext;

class DeferredPass {

  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;
  vk::UniquePipeline mPipeline;
  vk::UniqueFramebuffer mFramebuffer;

 public:
  DeferredPass(VulkanContext &context);

  DeferredPass(DeferredPass const &other) = delete;
  DeferredPass &operator=(DeferredPass const &other) = delete;

  DeferredPass(DeferredPass &&other) = default;
  DeferredPass&operator=(DeferredPass &&other) = default;

  void initializePipeline(std::vector<vk::DescriptorSetLayout> const &layouts,
                          std::vector<vk::Format> const &outputFormats);
  void initializeFramebuffer(std::vector<vk::ImageView> const& outputImageViews,
                             vk::Extent2D const &extent);

  inline vk::Framebuffer getFramebuffer() {
    return mFramebuffer.get();
  }
  inline vk::RenderPass getRenderPass() {
    return mRenderPass.get();
  }
  inline vk::PipelineLayout getPipelineLayout() {
    return mPipelineLayout.get();
  }
  inline vk::Pipeline getPipeline() {
    return mPipeline.get();
  }
};

}
