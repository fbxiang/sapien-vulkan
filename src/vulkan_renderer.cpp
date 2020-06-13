#include "vulkan_renderer.h"
#include "pass/gbuffer.h"
#include "vulkan_context.h"
#include "scene.h"
#include "camera.h"

namespace svulkan {

VulkanRenderer::VulkanRenderer(VulkanContext &context): mContext(&context) {
  mGBufferPass = std::make_unique<GBufferPass>(context);
}

void VulkanRenderer::resize(int width, int height) {
  log::info("Resizing renderer to {} x {}", width, height);
  mWidth = width;
  mHeight = height;

  initializeRenderTextures();
  initializeRenderPasses();
}

void VulkanRenderer::initializeRenderTextures() {
  mRenderTargetFormats.colorFormat = vk::Format::eR8G8B8A8Unorm;
  mRenderTargetFormats.depthFormat = vk::Format::eD32Sfloat;

  mRenderTargets.albedo = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.colorFormat, vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);

  mRenderTargets.position = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.colorFormat, vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);

  mRenderTargets.specular = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.colorFormat, vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);

  mRenderTargets.normal = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.colorFormat, vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);

  mRenderTargets.depth = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.depthFormat, vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);

  mRenderTargets.lighting = std::make_unique<VulkanImageData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      mRenderTargetFormats.colorFormat, vk::Extent2D(mWidth, mHeight), 1, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
      vk::ImageUsageFlagBits::eTransferSrc,
      vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);


  auto commandBuffer = createCommandBuffer(mContext->getDevice(), mContext->getCommandPool(),
                                           vk::CommandBufferLevel::ePrimary);

  OneTimeSubmit(mContext->getDevice(), mContext->getCommandPool(), mContext->getGraphicsQueue(), [this](vk::CommandBuffer commandBuffer) {
      
      transitionImageLayout(commandBuffer, mRenderTargets.albedo.get()->mImage.get(),
                            mRenderTargetFormats.colorFormat,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 
                            {},
                            vk::AccessFlagBits::eColorAttachmentRead |
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits::eTopOfPipe,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);
      transitionImageLayout(commandBuffer, mRenderTargets.position.get()->mImage.get(),
                            mRenderTargetFormats.colorFormat,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 
                            {},
                            vk::AccessFlagBits::eColorAttachmentRead |
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits::eTopOfPipe,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);
      transitionImageLayout(commandBuffer, mRenderTargets.normal.get()->mImage.get(),
                            mRenderTargetFormats.colorFormat,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 
                            {},
                            vk::AccessFlagBits::eColorAttachmentRead |
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits::eTopOfPipe,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);
      transitionImageLayout(commandBuffer, mRenderTargets.lighting.get()->mImage.get(),
                            mRenderTargetFormats.colorFormat,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 
                            {},
                            vk::AccessFlagBits::eColorAttachmentRead |
                            vk::AccessFlagBits::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits::eTopOfPipe,
                            vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);
      transitionImageLayout(commandBuffer, mRenderTargets.depth.get()->mImage.get(),
                            mRenderTargetFormats.depthFormat,
                            vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 
                            {},
                            vk::AccessFlagBits::eDepthStencilAttachmentRead |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                            vk::PipelineStageFlagBits::eTopOfPipe,
                            vk::PipelineStageFlagBits::eEarlyFragmentTests |
                            vk::PipelineStageFlagBits::eLateFragmentTests,
                            vk::ImageAspectFlagBits::eDepth);
    });
}

void VulkanRenderer::initializeRenderPasses() {
  auto &l = mContext->getDescriptorSetLayouts();
  std::vector<vk::DescriptorSetLayout> layouts = {
    l.scene.get(), l.camera.get(),
    l.object.get(), l.material.get()};
  std::vector<vk::Format> colorFormats = {
    vk::Format::eR8G8B8A8Unorm,  // color
    vk::Format::eR8G8B8A8Unorm,  // position
    vk::Format::eR8G8B8A8Unorm,  // specular
    vk::Format::eR8G8B8A8Unorm   // normal
  };
  vk::Format depthFormat = vk::Format::eD32Sfloat;
  mGBufferPass->initializePipeline(layouts, colorFormats, depthFormat,
                                   vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise);

  assert(mWidth > 0 && mHeight > 0);

  mGBufferPass->initializeFramebuffer(
      {mRenderTargets.albedo->mImageView.get(),
       mRenderTargets.position->mImageView.get(),
       mRenderTargets.specular->mImageView.get(),
       mRenderTargets.normal->mImageView.get()},
      mRenderTargets.depth->mImageView.get(),
      {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)});
}


void VulkanRenderer::render(vk::CommandBuffer commandBuffer, Scene &scene, Camera &camera) {
  // sync object data to GPU
  scene.prepareObjectsForRender();
  for (auto obj : scene.getOpaqueObjects()) {
    obj->updateVulkanObject();
  }

  // sync camera data to GPU
  camera.updateUBO();

  std::vector<vk::ClearValue> clearValues;
  clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 1.f}));  // albedo
  clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));  // position
  clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));  // specular
  clearValues.push_back(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));  // normal
  clearValues.push_back(vk::ClearDepthStencilValue(1.0f, 0));  // depth

  vk::RenderPassBeginInfo renderPassBeginInfo {
    mGBufferPass->getRenderPass(), mGBufferPass->getFramebuffer(),
    vk::Rect2D({0,0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}),
    static_cast<uint32_t>(clearValues.size()), clearValues.data()};

  commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGBufferPass->getPipeline());
  commandBuffer.setViewport(0, {
      {0.f, 0.f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.f, 1.f}
    });
  commandBuffer.setScissor(0, {{{0, 0}, {static_cast<uint32_t>(mWidth), static_cast<uint32_t>(mHeight)}}});

  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGBufferPass->getPipelineLayout(),
                                   0, scene.getVulkanScene()->getDescriptorSet(), nullptr);
  commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGBufferPass->getPipelineLayout(),
                                   1, camera.mDescriptorSet.get(), nullptr);
  for (auto &obj : scene.getOpaqueObjects()) {
    auto vobj = obj->getVulkanObject();
    if (vobj) {
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGBufferPass->getPipelineLayout(),
                                       2, vobj->mDescriptorSet.get(), nullptr);
      commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGBufferPass->getPipelineLayout(),
                                       3, vobj->mMaterial->getDescriptorSet(), nullptr);

      commandBuffer.bindVertexBuffers(0, *vobj->mMesh->mVertexBuffer->mBuffer, {0});
      commandBuffer.bindIndexBuffer(*vobj->mMesh->mIndexBuffer->mBuffer, 0, vk::IndexType::eUint32);
      commandBuffer.drawIndexed(vobj->mMesh->mIndexCount, 1, 0, 0, 0);
    }
  }
  commandBuffer.endRenderPass();
}


void VulkanRenderer::display(vk::CommandBuffer commandBuffer, vk::Image swapchainImage, vk::Format swapchainFormat, uint32_t width, uint32_t height) {
  auto & img = mRenderTargets.albedo;

  transitionImageLayout(commandBuffer, img->mImage.get(), mRenderTargetFormats.colorFormat,
                        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal,
                        vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eTransferRead,
                        vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer,
                        vk::ImageAspectFlagBits::eColor);
  transitionImageLayout(commandBuffer, swapchainImage, swapchainFormat,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                        {}, vk::AccessFlagBits::eTransferWrite,
                        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                        vk::ImageAspectFlagBits::eColor);
  vk::ImageSubresourceLayers imageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
  vk::ImageBlit imageBlit(imageSubresourceLayers,
                          {{vk::Offset3D{0, 0, 0}, vk::Offset3D{mWidth, mHeight, 1}}},
                          imageSubresourceLayers,
                          {{vk::Offset3D{0, 0, 0},
                                 vk::Offset3D{static_cast<int>(width), static_cast<int>(height), 1}}});

  commandBuffer.blitImage(img->mImage.get(), vk::ImageLayout::eTransferSrcOptimal,
                          swapchainImage, vk::ImageLayout::eTransferDstOptimal, imageBlit,
                          vk::Filter::eNearest);

  transitionImageLayout(commandBuffer, img->mImage.get(), mRenderTargetFormats.colorFormat,
                        vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal, 
                        vk::AccessFlagBits::eTransferRead,
                        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                        vk::ImageAspectFlagBits::eColor);
  transitionImageLayout(commandBuffer, swapchainImage, swapchainFormat,
                        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, 
                        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
                        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands,
                        vk::ImageAspectFlagBits::eColor);
}


} // namespace svulkan
