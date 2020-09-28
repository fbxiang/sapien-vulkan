#pragma once
#include "sapien_vulkan/internal/vulkan.h"
#include <map>

namespace svulkan {
class VulkanContext;

class CompositePass {

  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipelineLayout mPipelineLayout;

  vk::Pipeline mPipeline;

  // vk::UniquePipeline mPipelineLighting;
  // vk::UniquePipeline mPipelineNormal;
  // vk::UniquePipeline mPipelineDepth;
  // vk::UniquePipeline mPipelineSegmentation;
  // vk::UniquePipeline mPipelineCustom;
  std::map<std::string, vk::UniquePipeline> mPipelines;

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

  void initializePipeline(std::string const &shaderDir,
                          std::vector<vk::DescriptorSetLayout> const &layouts,
                          std::vector<vk::Format> const &outputFormats,
                          std::vector<std::string> const &pipelineNames);

  void initializeFramebuffer(std::vector<vk::ImageView> const &outputImageViews,
                             vk::Extent2D const &extent);

  inline vk::Framebuffer getFramebuffer() { return mFramebuffer.get(); }
  inline vk::RenderPass getRenderPass() { return mRenderPass.get(); }
  inline vk::PipelineLayout getPipelineLayout() { return mPipelineLayout.get(); }
  inline vk::Pipeline getPipeline() { return mPipeline; }

  void switchToPipeline(std::string const &name);
};

} // namespace svulkan
