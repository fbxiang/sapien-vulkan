#include "sapien_vulkan/pass/transparency.h"
#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/uniform_buffers.h"

namespace svulkan
{

static vk::UniqueRenderPass createRenderPass(vk::Device device, std::vector<vk::Format> const& colorFormats,
                                             vk::Format depthFormat,
                                             vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eDontCare) {
  std::vector<vk::AttachmentDescription> attachmentDescriptions;

  for (uint32_t i = 0; i < colorFormats.size(); ++i) {
    assert(colorFormats[i] != vk::Format::eUndefined);
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(), colorFormats[i], vk::SampleCountFlagBits::e1, loadOp,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        // lighting attachment is currently an attachment, the others are currently input textures
        i == 0 ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eColorAttachmentOptimal));
  }

  assert(depthFormat != vk::Format::eUndefined);
  attachmentDescriptions.push_back(vk::AttachmentDescription(
      vk::AttachmentDescriptionFlags(), depthFormat, vk::SampleCountFlagBits::e1, loadOp,
      vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
      // depth was in the gbuffer texture
      vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal));

  std::vector<vk::AttachmentReference> colorAttachments;
  for (uint32_t i = 0; i < colorFormats.size(); ++i) {
    colorAttachments.emplace_back(i, vk::ImageLayout::eColorAttachmentOptimal);
  }
  vk::AttachmentReference depthAttachment(colorFormats.size(),
                                          vk::ImageLayout::eDepthStencilAttachmentOptimal);
  vk::SubpassDescription subpassDescription(
      vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics,
      0, nullptr, colorAttachments.size(), colorAttachments.data(), nullptr, &depthAttachment);

  return device.createRenderPassUnique(
      vk::RenderPassCreateInfo({}, attachmentDescriptions.size(), attachmentDescriptions.data(),
                               1, &subpassDescription));
}

static vk::UniquePipeline createGraphicsPipeline(std::string const &shaderDir,
                                                 vk::Device device, uint32_t numColorAttachments,
                                                   vk::CullModeFlags cullMode, vk::FrontFace frontFace,
                                                   vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass
                                                   ) {
  vk::UniquePipelineCache pipelineCache = device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());

  auto vsm = createShaderModule(device, shaderDir + "/transparency.vert.spv");
  auto fsm = createShaderModule(device, shaderDir + "/transparency.frag.spv");

  // TODO: clean up light count handling
  std::vector<vk::SpecializationMapEntry> entries = { vk::SpecializationMapEntry(0, 0, sizeof(uint32_t)),
    vk::SpecializationMapEntry(1, sizeof(uint32_t), sizeof(uint32_t)) };
  std::vector<uint32_t> data = { NumDirectionalLights, NumPointLights };
  vk::SpecializationInfo specializationInfo(static_cast<uint32_t>(entries.size()), entries.data(),
                                            2 * sizeof(uint32_t), data.data());

  std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos {
    vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                      vk::ShaderStageFlagBits::eVertex, vsm.get(), "main", nullptr),
    vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                      vk::ShaderStageFlagBits::eFragment, fsm.get(), "main",
                                      &specializationInfo)
  };

  // vertex input state
  std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
  vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
  vk::VertexInputBindingDescription vertexInputBindingDescription(0, sizeof(Vertex));
  auto &vertexInputAttributeFormatOffset = Vertex::getFormatOffset();
  vertexInputAttributeDescriptions.reserve(vertexInputAttributeFormatOffset.size());
  for (uint32_t i = 0; i < vertexInputAttributeFormatOffset.size(); i++) {
    vertexInputAttributeDescriptions.push_back(vk::VertexInputAttributeDescription(
        i, 0, vertexInputAttributeFormatOffset[i].first, vertexInputAttributeFormatOffset[i].second));
  }
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescriptions.size();
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

  // input assembly state
  vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
      vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

  // tessellation state
  // nullptr

  // viewport state 
  vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(vk::PipelineViewportStateCreateFlags(),
                                                                      1, nullptr, 1, nullptr);

  // rasterization state
  vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
      vk::PipelineRasterizationStateCreateFlags(), false, false, vk::PolygonMode::eFill, cullMode, frontFace,
      false, 0.0f, 0.0f, 0.0f, 1.0f);

  // multisample state
  vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;

  // stencil state
  vk::StencilOpState stencilOpState{};
  vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
      vk::PipelineDepthStencilStateCreateFlags(), true, true, vk::CompareOp::eLessOrEqual,
      false, false, stencilOpState, stencilOpState);

  // color blend state
  vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  std::vector<vk::PipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates;

  // enable blending for the first texture
  pipelineColorBlendAttachmentStates.push_back(vk::PipelineColorBlendAttachmentState(
      true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha,
      vk::BlendOp::eAdd, vk::BlendFactor::eOne,
      vk::BlendFactor::eZero, vk::BlendOp::eAdd, colorComponentFlags));
  for (uint32_t i = 1; i < numColorAttachments; ++i) {
    pipelineColorBlendAttachmentStates.push_back(vk::PipelineColorBlendAttachmentState(
        false, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eZero,
        vk::BlendFactor::eZero, vk::BlendOp::eAdd, colorComponentFlags));
  }

  vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
      vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eNoOp, numColorAttachments,
      pipelineColorBlendAttachmentStates.data(), {{1.0f, 1.0f, 1.0f, 1.0f}});

  // dynamic state
  vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(vk::PipelineDynamicStateCreateFlags(), 2,
                                                                    dynamicStates);

  // create pipeline
  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
      vk::PipelineCreateFlags(),
      pipelineShaderStageCreateInfos.size(), pipelineShaderStageCreateInfos.data(),
      &pipelineVertexInputStateCreateInfo,
      &pipelineInputAssemblyStateCreateInfo, nullptr, &pipelineViewportStateCreateInfo,
      &pipelineRasterizationStateCreateInfo, &pipelineMultisampleStateCreateInfo,
      &pipelineDepthStencilStateCreateInfo, &pipelineColorBlendStateCreateInfo,
      &pipelineDynamicStateCreateInfo, pipelineLayout, renderPass);
  return device.createGraphicsPipelineUnique(pipelineCache.get(), graphicsPipelineCreateInfo);
}


TransparencyPass::TransparencyPass(VulkanContext &context): mContext(&context) {}

void TransparencyPass::initializePipeline(
    std::string const &shaderDir,
    std::vector<vk::DescriptorSetLayout> const &layouts,
    std::vector<vk::Format> const &colorFormats,
    vk::Format depthFormat,
    vk::CullModeFlags cullMode,
    vk::FrontFace frontFace) {

    vk::PushConstantRange range{vk::ShaderStageFlagBits::eFragment, 0, sizeof(float)};
    mPipelineLayout = mContext->getDevice().createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo(
        vk::PipelineLayoutCreateFlags(), layouts.size(), layouts.data(), 1, &range));

  mRenderPass = createRenderPass(mContext->getDevice(), colorFormats, depthFormat,
                                 vk::AttachmentLoadOp::eLoad);
  mPipeline = createGraphicsPipeline(shaderDir, mContext->getDevice(), colorFormats.size(),
                                     cullMode, frontFace, mPipelineLayout.get(), mRenderPass.get());

}

void TransparencyPass::initializeFramebuffer(std::vector<vk::ImageView> const& colorImageViews,
                                        vk::ImageView depthImageView,
                                        vk::Extent2D const &extent) {
  mFramebuffer = createFramebuffer(
      mContext->getDevice(), mRenderPass.get(), colorImageViews, depthImageView, extent);
}

}
