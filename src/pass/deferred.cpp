#include "sapien_vulkan/pass/deferred.h"
#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/uniform_buffers.h"

namespace svulkan
{

static vk::UniqueRenderPass createRenderPass(vk::Device device, std::vector<vk::Format> const &outputFormats) {
  std::vector<vk::AttachmentDescription> attachmentDescriptions;
  for (vk::Format format : outputFormats) {
    assert(format != vk::Format::eUndefined);
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(), format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal));
  }
  std::vector<vk::AttachmentReference> attachments;
  for (uint32_t i = 0; i < outputFormats.size(); ++i) {
    attachments.emplace_back(i, vk::ImageLayout::eColorAttachmentOptimal);
  }
  vk::SubpassDescription subpassDescription {
    {}, vk::PipelineBindPoint::eGraphics, 0, nullptr,
    static_cast<uint32_t>(attachments.size()), attachments.data()
  };
  return device.createRenderPassUnique(vk::RenderPassCreateInfo{
      {}, static_cast<uint32_t>(attachmentDescriptions.size()), attachmentDescriptions.data(),
      1, &subpassDescription});
}

static vk::UniquePipeline createGraphicsPipeline(
    std::string const &shaderDir,
    vk::Device device, uint32_t numColorAttachments,
    vk::PipelineLayout pipelineLayout,
    vk::RenderPass renderPass) {
  vk::UniquePipelineCache pipelineCache = device.createPipelineCacheUnique({});

  auto vsm = createShaderModule(device, shaderDir + "/deferred.vert.spv");
  auto fsm = createShaderModule(device, shaderDir + "/deferred.frag.spv");

  // TODO: clean up light count handling
  std::vector<vk::SpecializationMapEntry> entries = { vk::SpecializationMapEntry(0, 0, sizeof(uint32_t)),
    vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t)) };
  std::vector<uint32_t> data = { NumDirectionalLights, NumPointLights };
  vk::SpecializationInfo specializationInfo(static_cast<uint32_t>(entries.size()), entries.data(),
                                            2 * sizeof(uint32_t), data.data());

  vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[2] = {
    vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex,
                                      vsm.get(), "main", nullptr),
    vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                      vk::ShaderStageFlagBits::eFragment, fsm.get(), "main",
                                      &specializationInfo)};

  std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
  vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;

  vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
      vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

  vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(),
                                                                      1, nullptr, 1, nullptr);

  vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
      vk::PipelineRasterizationStateCreateFlags(), false, false, vk::PolygonMode::eFill,
      vk::CullModeFlagBits::eFront, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

  vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;

  vk::StencilOpState stencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                                    vk::CompareOp::eAlways);

  vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
      vk::PipelineDepthStencilStateCreateFlags(), false, false, vk::CompareOp::eLessOrEqual, false, false,
      stencilOpState, stencilOpState);

  vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

  std::vector<vk::PipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates;
  for (uint32_t i = 0; i < numColorAttachments; ++i) {
    pipelineColorBlendAttachmentStates.push_back(vk::PipelineColorBlendAttachmentState(
        false, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero,
        vk::BlendFactor::eZero, vk::BlendOp::eAdd, colorComponentFlags));
  }
  vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
      vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eNoOp, numColorAttachments,
      pipelineColorBlendAttachmentStates.data(), {{1.0f, 1.0f, 1.0f, 1.0f}});

  vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), 2,
                                                                    dynamicStates);

  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
      vk::PipelineCreateFlags(), 2, pipelineShaderStageCreateInfos, &pipelineVertexInputStateCreateInfo,
      &pipelineInputAssemblyStateCreateInfo, nullptr, &pipelineViewportStateCreateInfo,
      &pipelineRasterizationStateCreateInfo, &pipelineMultisampleStateCreateInfo,
      &pipelineDepthStencilStateCreateInfo, &pipelineColorBlendStateCreateInfo,
      &pipelineDynamicStateCreateInfo, pipelineLayout, renderPass);
  
  return device.createGraphicsPipelineUnique(pipelineCache.get(), graphicsPipelineCreateInfo);
}

DeferredPass::DeferredPass(VulkanContext &context): mContext(&context) {}

void DeferredPass::initializePipeline(
    std::string shaderDir,
    std::vector<vk::DescriptorSetLayout> const &layouts,
    std::vector<vk::Format> const &outputFormats) {
  // contains information about what buffers 
  mPipelineLayout = mContext->getDevice().createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo(
      vk::PipelineLayoutCreateFlags(), layouts.size(), layouts.data()));

  mRenderPass = createRenderPass(mContext->getDevice(), outputFormats);

  mPipeline = createGraphicsPipeline(shaderDir, mContext->getDevice(), outputFormats.size(),
                                     mPipelineLayout.get(), mRenderPass.get());
}

void DeferredPass::initializeFramebuffer(std::vector<vk::ImageView> const& outputImageViews,
                                        vk::Extent2D const &extent) {
  mFramebuffer = createFramebuffer(
      mContext->getDevice(), mRenderPass.get(), outputImageViews, {}, extent);
}

}
