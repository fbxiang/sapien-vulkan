#pragma once
#include "vulkan.h"
#include "vulkan_renderer_config.h"

namespace svulkan {

class VulkanContext;

class VulkanRendererForEditor {
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
    std::unique_ptr<VulkanImageData> lighting2;  // ping pong buffer for lighting
  } mRenderTargets;

  struct RenderTargetFormats {
    vk::Format colorFormat;
    vk::Format segmentationFormat;
    vk::Format depthFormat;
  } mRenderTargetFormats;

  std::unique_ptr<class GBufferPass> mGBufferPass;
  std::unique_ptr<class DeferredPass> mDeferredPass;
  std::unique_ptr<class AxisPass> mAxisPass;
  std::unique_ptr<class TransparencyPass> mTransparencyPass;
  std::unique_ptr<class CompositePass> mCompositePass;

  vk::UniqueDescriptorSet mDeferredDescriptorSet;
  vk::UniqueSampler mDeferredSampler;

  vk::UniqueDescriptorSet mCompositeDescriptorSet;
  vk::UniqueSampler mCompositeSampler;

public:
  VulkanRendererForEditor(VulkanContext &context, VulkanRendererConfig const &config);

  VulkanRendererForEditor(VulkanRendererForEditor const &other) = delete;
  VulkanRendererForEditor &operator=(VulkanRendererForEditor const &other) = delete;

  VulkanRendererForEditor(VulkanRendererForEditor &&other) = default;
  VulkanRendererForEditor &operator=(VulkanRendererForEditor &&other) = default;

  void resize(int width, int height);
  void initializeRenderTextures();
  void initializeRenderPasses();

  void switchToLighting();
  void switchToNormal();
  void switchToDepth();
  void switchToSegmentation();

  void render(vk::CommandBuffer commandBuffer, class Scene &scene, class Camera &camera);
  /* blit image to screen */
  void display(vk::CommandBuffer commandBuffer, vk::Image swapchainImage,
               vk::Format swapchainFormat, uint32_t width, uint32_t height);

  std::vector<float> downloadAlbedo();
  std::vector<float> downloadPosition();
  std::vector<float> downloadSpecular();
  std::vector<float> downloadNormal();
  std::vector<uint32_t> downloadSegmentation();
  std::vector<float> downloadDepth();
  std::vector<float> downloadLighting();

  inline RenderTargets &getRenderTargets() { return mRenderTargets; }

  //=== axis drawing ===//
private:
  std::vector<glm::mat4> mAxesTransforms{};
  vk::UniqueDescriptorSet mAxesDescriptorSet {};
  std::unique_ptr<VulkanBufferData> mAxesUBO {};
  std::shared_ptr<VulkanMesh> mAxesMesh {};
  void updateAxisUBO();

public:
  inline void addAxes(glm::mat4 axesTransform) { mAxesTransforms.push_back(axesTransform); }
  void clearAxes() { mAxesTransforms.clear(); }
  uint32_t getMaxAxes() { return 16; }
  void prepareAxesResources();
};

} // namespace svulkan