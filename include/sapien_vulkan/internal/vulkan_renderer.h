#pragma once
#include "vulkan.h"
#include "vulkan_renderer_config.h"

namespace svulkan
{

class VulkanContext;

class VulkanRenderer {
  VulkanContext *mContext;
  VulkanRendererConfig mConfig;

  int mWidth, mHeight;

  struct RenderTargets {
    std::unique_ptr<VulkanImageData> albedo;
    std::unique_ptr<VulkanImageData> position;
    std::unique_ptr<VulkanImageData> specular;
    std::unique_ptr<VulkanImageData> normal;
    std::unique_ptr<VulkanImageData> segmentation;
    std::unique_ptr<VulkanImageData> depth;

    std::unique_ptr<VulkanImageData> lighting;
  } mRenderTargets;

  struct RenderTargetFormats {
    vk::Format colorFormat;
    vk::Format segmentationFormat;
    vk::Format depthFormat;
  } mRenderTargetFormats;

  std::unique_ptr<class GBufferPass> mGBufferPass;
  std::unique_ptr<class DeferredPass> mDeferredPass;
  vk::UniqueDescriptorSet mDeferredDescriptorSet;
  vk::UniqueSampler mDeferredSampler;

 public:
  VulkanRenderer(VulkanContext &context, VulkanRendererConfig const &config);

  VulkanRenderer(VulkanRenderer const &other) = delete;
  VulkanRenderer &operator=(VulkanRenderer const &other) = delete;

  VulkanRenderer(VulkanRenderer &&other) = default;
  VulkanRenderer &operator=(VulkanRenderer &&other) = default;

  void resize(int width, int height);
  void initializeRenderTextures();
  void initializeRenderPasses();

  void render(vk::CommandBuffer commandBuffer, class Scene &scene, class Camera &camera);
  /* blit image to screen */
  void display(vk::CommandBuffer commandBuffer, vk::Image swapchainImage, vk::Format swapchainFormat,
               uint32_t width, uint32_t height);

  std::vector<float> downloadAlbedo();
  std::vector<float> downloadPosition();
  std::vector<float> downloadSpecular();
  std::vector<float> downloadNormal();
  std::vector<uint32_t> downloadSegmentation();
  std::vector<float> downloadDepth();
  std::vector<float> downloadLighting();
};

}
