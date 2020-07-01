#pragma once
#include "sapien_vulkan/internal/vulkan.h"

namespace svulkan {
class VulkanContext;

class CompositePass {

  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;

  vk::Pipeline mPipeline;

  vk::UniquePipeline mPipelineLighting;
  vk::UniquePipeline mPipelineNormal;
  vk::UniquePipeline mPipelineDepth;
  vk::UniquePipeline mPipelineSegmentation;
  vk::UniqueFramebuffer mFramebuffer;

  std::string mShaderDir;
  std::vector<vk::DescriptorSetLayout> mLayouts;
  std::vector<vk::Format> mOutputFormats;

public:
  CompositePass(VulkanContext &context);

  CompositePass(CompositePass const &other) = delete;
  CompositePass &operator=(CompositePass const &other) = delete;

  CompositePass(CompositePass &&other) = default;
  CompositePass &operator=(CompositePass &&other) = default;

  void initializePipeline(std::string shaderDir,
                          std::vector<vk::DescriptorSetLayout> const &layouts,
                          std::vector<vk::Format> const &outputFormats);

  void initializeFramebuffer(std::vector<vk::ImageView> const &outputImageViews,
                             vk::Extent2D const &extent);

  inline vk::Framebuffer getFramebuffer() { return mFramebuffer.get(); }
  inline vk::RenderPass getRenderPass() { return mRenderPass.get(); }
  inline vk::PipelineLayout getPipelineLayout() { return mPipelineLayout.get(); }
  inline vk::Pipeline getPipeline() { return mPipeline; }

  void switchToNormalPipeline();
  void switchToLightingPipeline();
  void switchToDepthPipeline();
  void switchToSegmentationPipeline();
};

} // namespace svulkan
