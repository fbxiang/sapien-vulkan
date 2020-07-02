#include "sapien_vulkan/pass/axis.h"
#include "sapien_vulkan/internal/vulkan_context.h"

namespace svulkan {

static vk::UniqueRenderPass
createRenderPass(vk::Device device, std::vector<vk::Format> const &colorFormats,
                 vk::Format depthFormat,
                 vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eDontCare) {
  std::vector<vk::AttachmentDescription> attachmentDescriptions;
  for (vk::Format colorFormat : colorFormats) {
    assert(colorFormat != vk::Format::eUndefined);
    attachmentDescriptions.push_back(vk::AttachmentDescription(
        vk::AttachmentDescriptionFlags(), colorFormat, vk::SampleCountFlagBits::e1, loadOp,
        vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal));
  }
  assert(depthFormat != vk::Format::eUndefined);
  attachmentDescriptions.push_back(vk::AttachmentDescription(
      vk::AttachmentDescriptionFlags(), depthFormat, vk::SampleCountFlagBits::e1, loadOp,
      vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
      vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthStencilAttachmentOptimal));

  std::vector<vk::AttachmentReference> colorAttachments;
  for (uint32_t i = 0; i < colorFormats.size(); ++i) {
    colorAttachments.emplace_back(i, vk::ImageLayout::eColorAttachmentOptimal);
  }
  vk::AttachmentReference depthAttachment(colorFormats.size(),
                                          vk::ImageLayout::eDepthStencilAttachmentOptimal);
  vk::SubpassDescription subpassDescription(
      vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr,
      colorAttachments.size(), colorAttachments.data(), nullptr, &depthAttachment);

  std::array<vk::SubpassDependency, 2> dependencies;
  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependencies[0].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                  vk::AccessFlagBits::eColorAttachmentWrite;
  dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                  vk::AccessFlagBits::eColorAttachmentWrite;
  dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                  vk::AccessFlagBits::eColorAttachmentWrite;
  dependencies[1].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                  vk::AccessFlagBits::eColorAttachmentWrite;
  dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

  return device.createRenderPassUnique(vk::RenderPassCreateInfo(
      {}, attachmentDescriptions.size(), attachmentDescriptions.data(), 1, &subpassDescription,
      dependencies.size(), dependencies.data()));
}

static vk::UniquePipeline createGraphicsPipeline(std::string const &shaderDir, vk::Device device,
                                                 uint32_t numColorAttachments,
                                                 vk::CullModeFlags cullMode,
                                                 vk::FrontFace frontFace,
                                                 vk::PipelineLayout pipelineLayout,
                                                 vk::RenderPass renderPass, uint32_t maxNumAxes) {
  vk::UniquePipelineCache pipelineCache =
      device.createPipelineCacheUnique(vk::PipelineCacheCreateInfo());

  auto vsm = createShaderModule(device, shaderDir + "/axis.vert.spv");
  auto fsm = createShaderModule(device, shaderDir + "/axis.frag.spv");

  std::vector<vk::SpecializationMapEntry> entries = {
      vk::SpecializationMapEntry(0, 0, sizeof(uint32_t))};
  std::vector<uint32_t> data = {maxNumAxes};
  vk::SpecializationInfo specializationInfo(static_cast<uint32_t>(entries.size()), entries.data(),
                                            sizeof(uint32_t), data.data());

  std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos{
      vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                        vk::ShaderStageFlagBits::eVertex, vsm.get(), "main",
                                        &specializationInfo),
      vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(),
                                        vk::ShaderStageFlagBits::eFragment, fsm.get(), "main",
                                        nullptr)};

  // vertex input state
  std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions;
  vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
  vk::VertexInputBindingDescription vertexInputBindingDescription(0, sizeof(Vertex));
  auto &vertexInputAttributeFormatOffset = Vertex::getFormatOffset();
  vertexInputAttributeDescriptions.reserve(vertexInputAttributeFormatOffset.size());
  for (uint32_t i = 0; i < vertexInputAttributeFormatOffset.size(); i++) {
    vertexInputAttributeDescriptions.push_back(
        vk::VertexInputAttributeDescription(i, 0, vertexInputAttributeFormatOffset[i].first,
                                            vertexInputAttributeFormatOffset[i].second));
  }
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
      vertexInputAttributeDescriptions.size();
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      vertexInputAttributeDescriptions.data();

  // input assembly state
  vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
      vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

  // tessellation state
  // nullptr

  // viewport state
  vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
      vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

  // rasterization state
  vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
      vk::PipelineRasterizationStateCreateFlags(), false, false, vk::PolygonMode::eFill, cullMode,
      frontFace, false, 0.0f, 0.0f, 0.0f, 1.0f);

  // multisample state
  vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;

  // stencil state
  vk::StencilOpState stencilOpState{};
  vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
      vk::PipelineDepthStencilStateCreateFlags(), true, true, vk::CompareOp::eLessOrEqual, false,
      false, stencilOpState, stencilOpState);

  // color blend state
  vk::ColorComponentFlags colorComponentFlags(
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  std::vector<vk::PipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates;
  for (uint32_t i = 0; i < numColorAttachments; ++i) {
    pipelineColorBlendAttachmentStates.push_back(vk::PipelineColorBlendAttachmentState(
        false, vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd, colorComponentFlags));
  }
  vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
      vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eNoOp, numColorAttachments,
      pipelineColorBlendAttachmentStates.data(), {{1.0f, 1.0f, 1.0f, 1.0f}});

  // dynamic state
  vk::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
      vk::PipelineDynamicStateCreateFlags(), 2, dynamicStates);

  // create pipeline
  vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo(
      vk::PipelineCreateFlags(), pipelineShaderStageCreateInfos.size(),
      pipelineShaderStageCreateInfos.data(), &pipelineVertexInputStateCreateInfo,
      &pipelineInputAssemblyStateCreateInfo, nullptr, &pipelineViewportStateCreateInfo,
      &pipelineRasterizationStateCreateInfo, &pipelineMultisampleStateCreateInfo,
      &pipelineDepthStencilStateCreateInfo, &pipelineColorBlendStateCreateInfo,
      &pipelineDynamicStateCreateInfo, pipelineLayout, renderPass);
  return device.createGraphicsPipelineUnique(pipelineCache.get(), graphicsPipelineCreateInfo);
}

AxisPass::AxisPass(VulkanContext &context) : mContext(&context) {}

void AxisPass::initializePipeline(std::string const &shaderDir,
                                  std::vector<vk::DescriptorSetLayout> const &layouts,
                                  std::vector<vk::Format> const &colorFormats,
                                  vk::Format depthFormat, vk::CullModeFlags cullMode,
                                  vk::FrontFace frontFace, uint32_t maxNumAxes) {
  // contains information about what buffers
  mPipelineLayout = mContext->getDevice().createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo(
      vk::PipelineLayoutCreateFlags(), layouts.size(), layouts.data()));

  mRenderPass = createRenderPass(mContext->getDevice(), colorFormats, depthFormat,
                                 vk::AttachmentLoadOp::eLoad);
  mPipeline =
      createGraphicsPipeline(shaderDir, mContext->getDevice(), colorFormats.size(), cullMode,
                             frontFace, mPipelineLayout.get(), mRenderPass.get(), maxNumAxes);
}

void AxisPass::initializeFramebuffer(std::vector<vk::ImageView> const &colorImageViews,
                                     vk::ImageView depthImageView, vk::Extent2D const &extent) {
  mFramebuffer = createFramebuffer(mContext->getDevice(), mRenderPass.get(), colorImageViews,
                                   depthImageView, extent);
}

} // namespace svulkan
