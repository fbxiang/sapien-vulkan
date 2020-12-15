#include "sapien_vulkan/internal/vulkan_renderer_for_editor.h"
#include "sapien_vulkan/camera.h"
#include "sapien_vulkan/data/geometry.hpp"
#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/pass/axis.h"
#include "sapien_vulkan/pass/composite.h"
#include "sapien_vulkan/pass/deferred.h"
#include "sapien_vulkan/pass/gbuffer.h"
#include "sapien_vulkan/pass/transparency.h"
#include "sapien_vulkan/scene.h"

namespace svulkan {

VulkanRendererForEditor::VulkanRendererForEditor(VulkanContext &context,
                                                 VulkanRendererConfig const &config)
    : mContext(&context), mConfig(config) {
  mGBufferPass = std::make_unique<GBufferPass>(context);
  mDeferredPass = std::make_unique<DeferredPass>(context);
  mAxisPass = std::make_unique<AxisPass>(context);
  mTransparencyPass = std::make_unique<TransparencyPass>(context);
  mCompositePass = std::make_unique<CompositePass>(context);

  initializeDescriptorLayouts();

  mDeferredDescriptorSet =
      std::move(mContext->getDevice()
                    .allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(
                        mContext->getDescriptorPool(), 1, &mDescriptorSetLayouts.deferred.get()))
                    .front());
  prepareAxesResources();
  prepareStickResources();
  mCompositeDescriptorSet =
      std::move(mContext->getDevice()
                    .allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(
                        mContext->getDescriptorPool(), 1, &mDescriptorSetLayouts.composite.get()))
                    .front());
  mInputTextures.resize(mConfig.customInputTextureCount);
  for (uint32_t i = 0; i < mConfig.customInputTextureCount; ++i) {
    mInputTextures[i] = context.getResourcesManager().getPlaceholderTexture();
  }
}

void VulkanRendererForEditor::loadCustomTexture(uint32_t index, std::string const &filename) {
  mInputTextures[index] = mContext->getResourcesManager().loadTexture(filename);
  if (mWidth) {
    initializeRenderTextures();
    initializeRenderPasses();
  }
}

void VulkanRendererForEditor::resize(int width, int height) {
  log::info("Resizing renderer to {} x {}", width, height);
  mWidth = width;
  mHeight = height;

  initializeRenderTextures();
  initializeRenderPasses();
}

void VulkanRendererForEditor::initializeRenderTextures() {
  mRenderTargetFormats.colorFormat = vk::Format::eR32G32B32A32Sfloat;
  mRenderTargetFormats.segmentationFormat = vk::Format::eR32G32B32A32Uint;
  mRenderTargetFormats.depthFormat = vk::Format::eD32Sfloat;

  mRenderTargets.albedo = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.position = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.specular = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.normal = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.segmentation = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.segmentationFormat, vk::Extent2D(mWidth, mHeight), 1,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.depth = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.depthFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eDepth);

  mRenderTargets.lighting = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.lighting2 = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
      vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
          vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
      vk::ImageAspectFlagBits::eColor);

  mRenderTargets.custom.resize(mConfig.customTextureCount);
  for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
    mRenderTargets.custom[i] = std::make_unique<VulkanImageData>(
        mContext->getPhysicalDevice(), mContext->getDevice(), mRenderTargetFormats.colorFormat,
        vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc,
        vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor);
  }

  auto commandBuffer = createCommandBuffer(mContext->getDevice(), mContext->getCommandPool(),
                                           vk::CommandBufferLevel::ePrimary);

  OneTimeSubmit(
      mContext->getDevice(), mContext->getCommandPool(), mContext->getGraphicsQueue(),
      [this](vk::CommandBuffer commandBuffer) {
        transitionImageLayout(
            commandBuffer, mRenderTargets.albedo.get()->mImage.get(),
            mRenderTargetFormats.colorFormat, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {},
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
        transitionImageLayout(
            commandBuffer, mRenderTargets.position.get()->mImage.get(),
            mRenderTargetFormats.colorFormat, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {},
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
        transitionImageLayout(
            commandBuffer, mRenderTargets.normal.get()->mImage.get(),
            mRenderTargetFormats.colorFormat, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {},
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
        transitionImageLayout(
            commandBuffer, mRenderTargets.lighting.get()->mImage.get(),
            mRenderTargetFormats.colorFormat, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {},
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
        transitionImageLayout(
            commandBuffer, mRenderTargets.lighting2.get()->mImage.get(),
            mRenderTargetFormats.colorFormat, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {},
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
        transitionImageLayout(commandBuffer, mRenderTargets.depth.get()->mImage.get(),
                              mRenderTargetFormats.depthFormat, vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eDepthStencilAttachmentOptimal, {},
                              vk::AccessFlagBits::eDepthStencilAttachmentRead |
                                  vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                              vk::PipelineStageFlagBits::eTopOfPipe,
                              vk::PipelineStageFlagBits::eEarlyFragmentTests |
                                  vk::PipelineStageFlagBits::eLateFragmentTests,
                              vk::ImageAspectFlagBits::eDepth);
        for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
          transitionImageLayout(
              commandBuffer, mRenderTargets.custom[i].get()->mImage.get(),
              mRenderTargetFormats.colorFormat, vk::ImageLayout::eUndefined,
              vk::ImageLayout::eColorAttachmentOptimal, {},
              vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
              vk::PipelineStageFlagBits::eTopOfPipe,
              vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
        }
      });

  // bind textures to deferred descriptor set
  mDeferredSampler = mContext->getDevice().createSamplerUnique(vk::SamplerCreateInfo(
      vk::SamplerCreateFlags(), vk::Filter::eNearest, vk::Filter::eNearest,
      vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToBorder,
      vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder, 0.f, false,
      0.f, false, vk::CompareOp::eNever, 0.f, 0.f, vk::BorderColor::eFloatOpaqueBlack));
  std::vector<vk::DescriptorImageInfo> imageInfos = {
      vk::DescriptorImageInfo(mDeferredSampler.get(), mRenderTargets.albedo->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mDeferredSampler.get(), mRenderTargets.position->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mDeferredSampler.get(), mRenderTargets.specular->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mDeferredSampler.get(), mRenderTargets.normal->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mDeferredSampler.get(), mRenderTargets.depth->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal)};
  for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
    imageInfos.push_back(vk::DescriptorImageInfo(mDeferredSampler.get(),
                                                 mRenderTargets.custom[i]->mImageView.get(),
                                                 vk::ImageLayout::eShaderReadOnlyOptimal));
  }
  for (uint32_t i = 0; i < mConfig.customInputTextureCount; ++i) {
    imageInfos.push_back(vk::DescriptorImageInfo(mInputTextures[i]->mTextureSampler.get(),
                                                 mInputTextures[i]->mImageData->mImageView.get(),
                                                 vk::ImageLayout::eShaderReadOnlyOptimal));
  }
  std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {vk::WriteDescriptorSet(
      mDeferredDescriptorSet.get(), 0, 0, imageInfos.size(),
      vk::DescriptorType::eCombinedImageSampler, imageInfos.data(), nullptr, nullptr)};
  mContext->getDevice().updateDescriptorSets(writeDescriptorSets, nullptr);

  // bind textures to composite descriptor set
  mCompositeSampler = mContext->getDevice().createSamplerUnique(vk::SamplerCreateInfo(
      vk::SamplerCreateFlags(), vk::Filter::eNearest, vk::Filter::eNearest,
      vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToBorder,
      vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder, 0.f, false,
      0.f, false, vk::CompareOp::eNever, 0.f, 0.f, vk::BorderColor::eFloatOpaqueBlack));
  imageInfos = {
      vk::DescriptorImageInfo(mCompositeSampler.get(), mRenderTargets.lighting->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mCompositeSampler.get(), mRenderTargets.albedo->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mCompositeSampler.get(), mRenderTargets.position->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mCompositeSampler.get(), mRenderTargets.specular->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mCompositeSampler.get(), mRenderTargets.normal->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mCompositeSampler.get(),
                              mRenderTargets.segmentation->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal),
      vk::DescriptorImageInfo(mCompositeSampler.get(), mRenderTargets.depth->mImageView.get(),
                              vk::ImageLayout::eShaderReadOnlyOptimal)};
  for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
    imageInfos.push_back(vk::DescriptorImageInfo(mDeferredSampler.get(),
                                                 mRenderTargets.custom[i]->mImageView.get(),
                                                 vk::ImageLayout::eShaderReadOnlyOptimal));
  }
  writeDescriptorSets = {vk::WriteDescriptorSet(
      mCompositeDescriptorSet.get(), 0, 0, imageInfos.size(),
      vk::DescriptorType::eCombinedImageSampler, imageInfos.data(), nullptr, nullptr)};
  mContext->getDevice().updateDescriptorSets(writeDescriptorSets, nullptr);
}

void VulkanRendererForEditor::initializeRenderPasses() {
  assert(mWidth > 0 && mHeight > 0);

  auto &l = mContext->getDescriptorSetLayouts();
  std::string const shaderDir =
      mConfig.shaderDir == "" ? VulkanContext::gDefaultShaderDir : mConfig.shaderDir;

  vk::CullModeFlags cullMode;
  if (mConfig.culling == "back") {
    cullMode = vk::CullModeFlagBits::eBack;
  } else if (mConfig.culling == "front") {
    cullMode = vk::CullModeFlagBits::eFront;
  } else if (mConfig.culling == "both") {
    cullMode = vk::CullModeFlagBits::eFrontAndBack;
  } else if (mConfig.culling == "none" || mConfig.culling == "") {
    cullMode = vk::CullModeFlagBits::eNone;
  } else {
    log::warn("Unknown culling mode {}, default to None");
    cullMode = vk::CullModeFlagBits::eNone;
  }

  // initialize gbuffer pass
  {
    std::vector<vk::DescriptorSetLayout> layouts = {l.scene.get(), l.camera.get(), l.object.get(),
                                                    l.material.get()};
    std::vector<vk::Format> colorFormats = {
        mRenderTargetFormats.colorFormat, mRenderTargetFormats.colorFormat,
        mRenderTargetFormats.colorFormat, mRenderTargetFormats.colorFormat,
        mRenderTargetFormats.segmentationFormat};

    std::vector<vk::ImageView> imageViews = {
        mRenderTargets.albedo->mImageView.get(), mRenderTargets.position->mImageView.get(),
        mRenderTargets.specular->mImageView.get(), mRenderTargets.normal->mImageView.get(),
        mRenderTargets.segmentation->mImageView.get()};

    for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
      colorFormats.push_back(mRenderTargetFormats.colorFormat);
      imageViews.push_back(mRenderTargets.custom[i]->mImageView.get());
    }

    mGBufferPass->initializePipeline(shaderDir, layouts, colorFormats,
                                     mRenderTargetFormats.depthFormat, cullMode,
                                     vk::FrontFace::eCounterClockwise);
    mGBufferPass->initializeFramebuffer(
        imageViews, mRenderTargets.depth->mImageView.get(),
        {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)});
  }

  // initialize deferred pass
  {
    std::vector<vk::DescriptorSetLayout> layouts = {l.scene.get(), l.camera.get(),
                                                    mDescriptorSetLayouts.deferred.get()};
    mDeferredPass->initializePipeline(shaderDir, layouts, {mRenderTargetFormats.colorFormat});
    mDeferredPass->initializeFramebuffer(
        {mRenderTargets.lighting->mImageView.get()},
        {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)});
  }

  // initialize axis pass
  {
    mAxisPass->initializePipeline(shaderDir, {l.object.get(), l.camera.get()},
                                  {mRenderTargetFormats.colorFormat},
                                  mRenderTargetFormats.depthFormat, cullMode,
                                  vk::FrontFace::eCounterClockwise, getMaxAxisPassInstances());
    mAxisPass->initializeFramebuffer(
        {mRenderTargets.lighting->mImageView.get()}, mRenderTargets.depth->mImageView.get(),
        {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)});
  }

  // initialize transparency pass
  {
    std::vector<vk::DescriptorSetLayout> layouts = {l.scene.get(), l.camera.get(), l.object.get(),
                                                    l.material.get()};
    std::vector<vk::Format> colorFormats = {
        mRenderTargetFormats.colorFormat, mRenderTargetFormats.colorFormat,
        mRenderTargetFormats.colorFormat, mRenderTargetFormats.colorFormat,
        mRenderTargetFormats.colorFormat, mRenderTargetFormats.segmentationFormat};
    std::vector<vk::ImageView> imageViews = {
        mRenderTargets.lighting->mImageView.get(), mRenderTargets.albedo->mImageView.get(),
        mRenderTargets.position->mImageView.get(), mRenderTargets.specular->mImageView.get(),
        mRenderTargets.normal->mImageView.get(),   mRenderTargets.segmentation->mImageView.get()};
    for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
      colorFormats.push_back(mRenderTargetFormats.colorFormat);
      imageViews.push_back(mRenderTargets.custom[i]->mImageView.get());
    }

    mTransparencyPass->initializePipeline(shaderDir, layouts, colorFormats,
                                          mRenderTargetFormats.depthFormat, cullMode,
                                          vk::FrontFace::eCounterClockwise);
    mTransparencyPass->initializeFramebuffer(
        imageViews, mRenderTargets.depth->mImageView.get(),
        {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)});
  }

  // initialize composite pass
  {
    std::vector<vk::DescriptorSetLayout> layouts = {mDescriptorSetLayouts.composite.get()};
    mCompositePass->initializePipeline(shaderDir, layouts, {mRenderTargetFormats.colorFormat},
                                       {"composite", "composite_normal", "composite_depth",
                                        "composite_segmentation", "composite_custom"});
    mCompositePass->initializeFramebuffer(
        {mRenderTargets.lighting2->mImageView.get()},
        {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)});
  }
}

void VulkanRendererForEditor::render(vk::CommandBuffer commandBuffer, Scene &scene,
                                     Camera &camera) {
  // sync object data to GPU
  scene.prepareObjectsForRender();
  for (auto obj : scene.getOpaqueObjects()) {
    obj->updateVulkanObject();
  }
  for (auto obj : scene.getTransparentObjects()) {
    obj->updateVulkanObject();
  }

  // sync camera and scene info to GPU
  camera.updateUBO();
  scene.updateUBO();

  // sync axes transform
  updateAxisUBO();
  updateStickUBO();

  // render gbuffer pass
  {
    std::vector<vk::ClearValue> clearValues;
    clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 1.f})); // albedo
    clearValues.push_back(
        vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f})); // position
    clearValues.push_back(
        vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f})); // specular
    clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f})); // normal
    clearValues.push_back(
        vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f})); // segmentation
    for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
      clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));
    }
    clearValues.push_back(vk::ClearDepthStencilValue(1.0f, 0)); // depth

    vk::RenderPassBeginInfo renderPassBeginInfo{
        mGBufferPass->getRenderPass(), mGBufferPass->getFramebuffer(),
        vk::Rect2D({0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}),
        static_cast<uint32_t>(clearValues.size()), clearValues.data()};

    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGBufferPass->getPipeline());
    commandBuffer.setViewport(
        0, {{0.f, 0.f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.f, 1.f}});
    commandBuffer.setScissor(
        0, {{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}}});

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mGBufferPass->getPipelineLayout(), 0,
                                     scene.getVulkanScene()->getDescriptorSet(), nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mGBufferPass->getPipelineLayout(), 1,
                                     camera.mDescriptorSet.get(), nullptr);
    for (auto &obj : scene.getOpaqueObjects()) {
      auto vobj = obj->getVulkanObject();
      if (vobj) {
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         mGBufferPass->getPipelineLayout(), 2,
                                         vobj->mDescriptorSet.get(), nullptr);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         mGBufferPass->getPipelineLayout(), 3,
                                         vobj->mMaterial->getDescriptorSet(), nullptr);

        commandBuffer.bindVertexBuffers(0, *vobj->mMesh->mVertexBuffer->mBuffer, {0});
        commandBuffer.bindIndexBuffer(*vobj->mMesh->mIndexBuffer->mBuffer, 0,
                                      vk::IndexType::eUint32);
        commandBuffer.drawIndexed(vobj->mMesh->mIndexCount, 1, 0, 0, 0);
      }
    }
    commandBuffer.endRenderPass();
  }

  // render deferred pass
  {
    // transition to texture formats
    // for (auto img : {mRenderTargets.albedo->mImage.get(), mRenderTargets.position->mImage.get(),
    //                  mRenderTargets.specular->mImage.get(),
    //                  mRenderTargets.normal->mImage.get()}) {
    //   transitionImageLayout(
    //       commandBuffer, img, mRenderTargetFormats.colorFormat,
    //       vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
    //       vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
    //       vk::PipelineStageFlagBits::eColorAttachmentOutput,
    //       vk::PipelineStageFlagBits::eFragmentShader, vk::ImageAspectFlagBits::eColor);
    // }
    // transitionImageLayout(
    //     commandBuffer, mRenderTargets.depth->mImage.get(), mRenderTargetFormats.depthFormat,
    //     vk::ImageLayout::eDepthStencilAttachmentOptimal,
    //     vk::ImageLayout::eShaderReadOnlyOptimal,
    //     vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead,
    //     vk::PipelineStageFlagBits::eEarlyFragmentTests |
    //         vk::PipelineStageFlagBits::eLateFragmentTests,
    //     vk::PipelineStageFlagBits::eFragmentShader, vk::ImageAspectFlagBits::eDepth);

    // draw quad
    std::vector<vk::ClearValue> clearValues = {
        vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}}};
    vk::RenderPassBeginInfo renderPassBeginInfo{
        mDeferredPass->getRenderPass(), mDeferredPass->getFramebuffer(),
        vk::Rect2D{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}},
        static_cast<uint32_t>(clearValues.size()), clearValues.data()};
    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mDeferredPass->getPipeline());
    commandBuffer.setViewport(
        0, {{0.f, 0.f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.f, 1.f}});
    commandBuffer.setScissor(
        0, {{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}}});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mDeferredPass->getPipelineLayout(), 0,
                                     scene.getVulkanScene()->getDescriptorSet(), nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mDeferredPass->getPipelineLayout(), 1,
                                     camera.mDescriptorSet.get(), nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mDeferredPass->getPipelineLayout(), 2,
                                     mDeferredDescriptorSet.get(), nullptr);

    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRenderPass();

    // transition textures back to gbuffer formats
    // for (auto img : {mRenderTargets.albedo->mImage.get(), mRenderTargets.position->mImage.get(),
    //                  mRenderTargets.specular->mImage.get(),
    //                  mRenderTargets.normal->mImage.get()}) {
    //   transitionImageLayout(
    //       commandBuffer, img, mRenderTargetFormats.colorFormat,
    //       vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal,
    //       vk::AccessFlagBits::eShaderRead,
    //       vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
    //       vk::PipelineStageFlagBits::eFragmentShader,
    //       vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
    // }
    // transitionImageLayout(
    //     commandBuffer, mRenderTargets.depth->mImage.get(), mRenderTargetFormats.depthFormat,
    //     vk::ImageLayout::eShaderReadOnlyOptimal,
    //     vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlagBits::eShaderRead,
    //     vk::AccessFlagBits::eDepthStencilAttachmentRead |
    //         vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    //     vk::PipelineStageFlagBits::eFragmentShader,
    //     vk::PipelineStageFlagBits::eEarlyFragmentTests |
    //         vk::PipelineStageFlagBits::eLateFragmentTests,
    //     vk::ImageAspectFlagBits::eDepth);
  }

  // axis pass
  if (mAxesTransforms.size() || mStickTransforms.size()) {
    std::vector<vk::ClearValue> clearValues;
    clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 1.f})); // albedo
    clearValues.push_back(vk::ClearDepthStencilValue(1.0f, 0));                           // depth

    vk::RenderPassBeginInfo renderPassBeginInfo{
        mAxisPass->getRenderPass(), mAxisPass->getFramebuffer(),
        vk::Rect2D({0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}),
        static_cast<uint32_t>(clearValues.size()), clearValues.data()};

    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mAxisPass->getPipeline());
    commandBuffer.setViewport(
        0, {{0.f, 0.f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.f, 1.f}});
    commandBuffer.setScissor(
        0, {{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}}});

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mAxisPass->getPipelineLayout(), 1,
                                     camera.mDescriptorSet.get(), nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mAxisPass->getPipelineLayout(), 0, mAxesDescriptorSet.get(),
                                     nullptr);
    commandBuffer.bindVertexBuffers(0, mAxesMesh->mVertexBuffer->mBuffer.get(), {0});
    commandBuffer.bindIndexBuffer(mAxesMesh->mIndexBuffer->mBuffer.get(), 0,
                                  vk::IndexType::eUint32);
    commandBuffer.drawIndexed(
        mAxesMesh->mIndexCount,
        std::min(static_cast<uint32_t>(mAxesTransforms.size()), getMaxAxisPassInstances()), 0, 0,
        0);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mAxisPass->getPipelineLayout(), 0, mStickDescriptorSet.get(),
                                     nullptr);
    commandBuffer.bindVertexBuffers(0, mStickMesh->mVertexBuffer->mBuffer.get(), {0});
    commandBuffer.bindIndexBuffer(mStickMesh->mIndexBuffer->mBuffer.get(), 0,
                                  vk::IndexType::eUint32);
    commandBuffer.drawIndexed(
        mStickMesh->mIndexCount,
        std::min(static_cast<uint32_t>(mStickTransforms.size()), getMaxAxisPassInstances()), 0, 0,
        0);

    commandBuffer.endRenderPass();
  }

  // transparency pass
  std::vector<vk::ClearValue> clearValues;
  clearValues.resize(7 + mConfig.customTextureCount);
  vk::RenderPassBeginInfo renderPassBeginInfo{
      mTransparencyPass->getRenderPass(), mTransparencyPass->getFramebuffer(),
      vk::Rect2D({0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}),
      static_cast<uint32_t>(clearValues.size()), clearValues.data()};

  commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mTransparencyPass->getPipeline());
  commandBuffer.setViewport(
      0, {{0.f, 0.f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.f, 1.f}});
  commandBuffer.setScissor(
      0, {{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}}});

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   mTransparencyPass->getPipelineLayout(), 0,
                                   scene.getVulkanScene()->getDescriptorSet(), nullptr);
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   mTransparencyPass->getPipelineLayout(), 1,
                                   camera.mDescriptorSet.get(), nullptr);
  for (auto &obj : scene.getTransparentObjects()) {
    auto vobj = obj->getVulkanObject();
    if (vobj) {
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       mTransparencyPass->getPipelineLayout(), 2,
                                       vobj->mDescriptorSet.get(), nullptr);
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                       mTransparencyPass->getPipelineLayout(), 3,
                                       vobj->mMaterial->getDescriptorSet(), nullptr);
      commandBuffer.pushConstants<float>(mTransparencyPass->getPipelineLayout(),
                                         vk::ShaderStageFlagBits::eFragment, 0, obj->mVisibility);

      commandBuffer.bindVertexBuffers(0, *vobj->mMesh->mVertexBuffer->mBuffer, {0});
      commandBuffer.bindIndexBuffer(*vobj->mMesh->mIndexBuffer->mBuffer, 0,
                                    vk::IndexType::eUint32);
      commandBuffer.drawIndexed(vobj->mMesh->mIndexCount, 1, 0, 0, 0);
    }
  }
  commandBuffer.endRenderPass();

  // composite pass
  {
    // transition to texture formats
    for (auto img :
         {mRenderTargets.lighting->mImage.get(), mRenderTargets.albedo->mImage.get(),
          mRenderTargets.position->mImage.get(), mRenderTargets.specular->mImage.get(),
          mRenderTargets.normal->mImage.get(), mRenderTargets.segmentation->mImage.get()}) {
      transitionImageLayout(
          commandBuffer, img, mRenderTargetFormats.colorFormat,
          vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
          vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
          vk::PipelineStageFlagBits::eColorAttachmentOutput,
          vk::PipelineStageFlagBits::eFragmentShader, vk::ImageAspectFlagBits::eColor);
    }
    for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
      transitionImageLayout(
          commandBuffer, mRenderTargets.custom[i]->mImage.get(), mRenderTargetFormats.colorFormat,
          vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
          vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
          vk::PipelineStageFlagBits::eColorAttachmentOutput,
          vk::PipelineStageFlagBits::eFragmentShader, vk::ImageAspectFlagBits::eColor);
    }

    transitionImageLayout(
        commandBuffer, mRenderTargets.depth->mImage.get(), mRenderTargetFormats.depthFormat,
        vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead,
        vk::PipelineStageFlagBits::eEarlyFragmentTests |
            vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eFragmentShader, vk::ImageAspectFlagBits::eDepth);

    std::vector<vk::ClearValue> clearValues = {
        vk::ClearColorValue{std::array{0.f, 0.f, 0.f, 1.f}}};
    vk::RenderPassBeginInfo renderPassBeginInfo{
        mCompositePass->getRenderPass(), mCompositePass->getFramebuffer(),
        vk::Rect2D{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}},
        static_cast<uint32_t>(clearValues.size()), clearValues.data()};
    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mCompositePass->getPipeline());
    commandBuffer.setViewport(
        0, {{0.f, 0.f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.f, 1.f}});
    commandBuffer.setScissor(
        0, {{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}}});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     mCompositePass->getPipelineLayout(), 0,
                                     mCompositeDescriptorSet.get(), nullptr);

    commandBuffer.draw(3, 1, 0, 0);
    commandBuffer.endRenderPass();

    // transition textures back to gbuffer formats
    for (auto img :
         {mRenderTargets.lighting->mImage.get(), mRenderTargets.albedo->mImage.get(),
          mRenderTargets.position->mImage.get(), mRenderTargets.specular->mImage.get(),
          mRenderTargets.normal->mImage.get(), mRenderTargets.segmentation->mImage.get()}) {
      transitionImageLayout(
          commandBuffer, img, mRenderTargetFormats.colorFormat,
          vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal,
          vk::AccessFlagBits::eShaderRead,
          vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
          vk::PipelineStageFlagBits::eFragmentShader,
          vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
    }
    transitionImageLayout(
        commandBuffer, mRenderTargets.depth->mImage.get(), mRenderTargetFormats.depthFormat,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal,
        vk::AccessFlagBits::eShaderRead,
        vk::AccessFlagBits::eDepthStencilAttachmentRead |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eEarlyFragmentTests |
            vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth);
  }
}

void VulkanRendererForEditor::display(vk::CommandBuffer commandBuffer, vk::Image swapchainImage,
                                      vk::Format swapchainFormat, uint32_t width,
                                      uint32_t height) {
  auto &img = mRenderTargets.lighting2;

  transitionImageLayout(
      commandBuffer, img->mImage.get(), mRenderTargetFormats.colorFormat,
      vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal,
      vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eTransferRead,
      vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer,
      vk::ImageAspectFlagBits::eColor);
  transitionImageLayout(commandBuffer, swapchainImage, swapchainFormat,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, {},
                        vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTopOfPipe,
                        vk::PipelineStageFlagBits::eTransfer, vk::ImageAspectFlagBits::eColor);
  vk::ImageSubresourceLayers imageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
  vk::ImageBlit imageBlit(imageSubresourceLayers,
                          {{vk::Offset3D{0, 0, 0}, vk::Offset3D{mWidth, mHeight, 1}}},
                          imageSubresourceLayers,
                          {{vk::Offset3D{0, 0, 0},
                            vk::Offset3D{static_cast<int>(width), static_cast<int>(height), 1}}});

  commandBuffer.blitImage(img->mImage.get(), vk::ImageLayout::eTransferSrcOptimal, swapchainImage,
                          vk::ImageLayout::eTransferDstOptimal, imageBlit, vk::Filter::eNearest);

  transitionImageLayout(
      commandBuffer, img->mImage.get(), mRenderTargetFormats.colorFormat,
      vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal,
      vk::AccessFlagBits::eTransferRead,
      vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
      vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::ImageAspectFlagBits::eColor);
  transitionImageLayout(
      commandBuffer, swapchainImage, swapchainFormat, vk::ImageLayout::eTransferDstOptimal,
      vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eTransferWrite,
      vk::AccessFlagBits::eMemoryRead, vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eAllCommands, vk::ImageAspectFlagBits::eColor);
}

std::vector<float> VulkanRendererForEditor::downloadAlbedo() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                4 * sizeof(float);
  return mRenderTargets.albedo->download<float>(mContext->getPhysicalDevice(),
                                                mContext->getDevice(), mContext->getCommandPool(),
                                                mContext->getGraphicsQueue(), size);
}

std::vector<float> VulkanRendererForEditor::downloadPosition() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                4 * sizeof(float);
  return mRenderTargets.position->download<float>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
      mContext->getGraphicsQueue(), size);
}

std::vector<float> VulkanRendererForEditor::downloadSpecular() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                4 * sizeof(float);
  return mRenderTargets.specular->download<float>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
      mContext->getGraphicsQueue(), size);
}

std::vector<float> VulkanRendererForEditor::downloadNormal() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                4 * sizeof(float);
  return mRenderTargets.normal->download<float>(mContext->getPhysicalDevice(),
                                                mContext->getDevice(), mContext->getCommandPool(),
                                                mContext->getGraphicsQueue(), size);
}

std::vector<float> VulkanRendererForEditor::downloadLighting() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                4 * sizeof(float);
  return mRenderTargets.lighting->download<float>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
      mContext->getGraphicsQueue(), size);
}

std::vector<float> VulkanRendererForEditor::downloadDepth() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                sizeof(float);
  return mRenderTargets.depth->download<float>(mContext->getPhysicalDevice(),
                                               mContext->getDevice(), mContext->getCommandPool(),
                                               mContext->getGraphicsQueue(), size);
}

std::vector<uint32_t> VulkanRendererForEditor::downloadSegmentation() {
  size_t size = (mRenderTargets.albedo->mExtent.width * mRenderTargets.albedo->mExtent.height) *
                4 * sizeof(uint32_t);
  return mRenderTargets.segmentation->download<uint32_t>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
      mContext->getGraphicsQueue(), size);
}

void VulkanRendererForEditor::prepareAxesResources() {
  mAxesUBO = std::make_unique<VulkanBufferData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      getMaxAxisPassInstances() * sizeof(glm::mat4), vk::BufferUsageFlagBits::eUniformBuffer);

  mAxesDescriptorSet = std::move(
      mContext->getDevice()
          .allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(
              mContext->getDescriptorPool(), 1, &mContext->getDescriptorSetLayouts().object.get()))
          .front());
  updateDescriptorSets(mContext->getDevice(), mAxesDescriptorSet.get(),
                       {{vk::DescriptorType::eUniformBuffer, mAxesUBO->mBuffer.get(), {}}}, {}, 0);

  std::vector vertices = AxesVertices;
  std::vector indices = AxesIndices;
  mAxesMesh = std::make_shared<VulkanMesh>(mContext->getPhysicalDevice(), mContext->getDevice(),
                                           mContext->getCommandPool(),
                                           mContext->getGraphicsQueue(), vertices, indices, false);
}

void VulkanRendererForEditor::prepareStickResources() {
  mStickUBO = std::make_unique<VulkanBufferData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      getMaxAxisPassInstances() * sizeof(glm::mat4), vk::BufferUsageFlagBits::eUniformBuffer);

  mStickDescriptorSet = std::move(
      mContext->getDevice()
          .allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(
              mContext->getDescriptorPool(), 1, &mContext->getDescriptorSetLayouts().object.get()))
          .front());
  updateDescriptorSets(mContext->getDevice(), mStickDescriptorSet.get(),
                       {{vk::DescriptorType::eUniformBuffer, mStickUBO->mBuffer.get(), {}}}, {},
                       0);

  std::vector vertices = StickVertices;
  std::vector indices = StickIndices;
  mStickMesh = std::make_shared<VulkanMesh>(
      mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
      mContext->getGraphicsQueue(), vertices, indices, false);
}

void VulkanRendererForEditor::updateAxisUBO() {
  if (mAxesTransforms.size()) {
    copyToDevice<glm::mat4>(
        mContext->getDevice(), mAxesUBO->mMemory.get(), mAxesTransforms.data(),
        std::min(static_cast<uint32_t>(mAxesTransforms.size()), getMaxAxisPassInstances()));
  }
}

void VulkanRendererForEditor::updateStickUBO() {
  if (mStickTransforms.size()) {
    copyToDevice<glm::mat4>(
        mContext->getDevice(), mStickUBO->mMemory.get(), mStickTransforms.data(),
        std::min(static_cast<uint32_t>(mStickTransforms.size()), getMaxAxisPassInstances()));
  }
}

void VulkanRendererForEditor::switchToLighting() { mCompositePass->switchToPipeline("composite"); }

void VulkanRendererForEditor::switchToNormal() {
  mCompositePass->switchToPipeline("composite_normal");
}

void VulkanRendererForEditor::switchToDepth() {
  mCompositePass->switchToPipeline("composite_depth");
}

void VulkanRendererForEditor::switchToSegmentation() {
  mCompositePass->switchToPipeline("composite_segmentation");
}

void VulkanRendererForEditor::switchToCustom() {
  mCompositePass->switchToPipeline("composite_custom");
}

void VulkanRendererForEditor::initializeDescriptorLayouts() {

  std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> layout = {
      {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}, // albedo
      {vk::DescriptorType::eCombinedImageSampler, 1,
       vk::ShaderStageFlagBits::eFragment}, // position
      {vk::DescriptorType::eCombinedImageSampler, 1,
       vk::ShaderStageFlagBits::eFragment}, // specular
      {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}, // normal
      {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}  // depth
  };
  for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
    layout.push_back(
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment});
  }
  for (uint32_t i = 0; i < mConfig.customInputTextureCount; ++i) {
    layout.push_back(
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment});
  }
  mDescriptorSetLayouts.deferred = createDescriptorSetLayout(mContext->getDevice(), layout);

  layout = {
      {vk::DescriptorType::eCombinedImageSampler, 1,
       vk::ShaderStageFlagBits::eFragment}, // lighting
      {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}, // albedo
      {vk::DescriptorType::eCombinedImageSampler, 1,
       vk::ShaderStageFlagBits::eFragment}, // position
      {vk::DescriptorType::eCombinedImageSampler, 1,
       vk::ShaderStageFlagBits::eFragment}, // specular
      {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}, // normal
      {vk::DescriptorType::eCombinedImageSampler, 1,
       vk::ShaderStageFlagBits::eFragment}, // segmentation
      {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment} // depth
  };
  for (uint32_t i = 0; i < mConfig.customTextureCount; ++i) {
    layout.push_back(
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment});
  }
  mDescriptorSetLayouts.composite = createDescriptorSetLayout(mContext->getDevice(), layout);
}

} // namespace svulkan
