#pragma once
#include "common/vulkan.h"

namespace svulkan
{
class VulkanContext;

class GBufferPass {

  VulkanContext *mContext;
  vk::UniqueRenderPass mRenderPass;
  vk::UniquePipeline mPipeline;

  public:
  GBufferPass(VulkanContext &context);
  void initializePipeline(std::vector<vk::DescriptorSetLayout> const &layouts,
                          std::vector<vk::Format> colorFormats,
                          vk::CullModeFlags cullMode,
                          vk::FrontFace frontFace);
  
};

}
