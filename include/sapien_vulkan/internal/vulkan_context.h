#pragma once

#include "sapien_vulkan/common/glm_common.h"
#include "sapien_vulkan/pass/axis.h"
#include "sapien_vulkan/pass/composite.h"
#include "sapien_vulkan/pass/deferred.h"
#include "sapien_vulkan/pass/gbuffer.h"
#include "sapien_vulkan/pass/transparency.h"
#include "vulkan_renderer_config.h"
#include "vulkan_resources_manager.h"
#include <vulkan/vulkan.hpp>

#ifdef ON_SCREEN
#include "sapien_vulkan/gui/gui.h"
#endif

namespace svulkan {

class Object;
class VulkanRenderer;
class VulkanRendererForEditor;

class VulkanContext {
  bool mRequirePresent;
  uint32_t mObjectBufferSize;

  vk::PhysicalDevice mPhysicalDevice;
  vk::UniqueInstance mInstance;
  vk::UniqueDevice mDevice;
  vk::UniqueCommandPool mCommandPool;
  vk::UniqueDescriptorPool mDescriptorPool;

  uint32_t graphicsQueueFamilyIndex;

  VulkanResourcesManager mResourcesManager;


public:
  VulkanContext(bool requirePresent = true, uint32_t objectBufferSize = 1000);
  ~VulkanContext();

  /** Get the graphics queue */
  vk::Queue getGraphicsQueue() const;

  inline uint32_t getGraphicsQueueFamilyIndex() { return graphicsQueueFamilyIndex; }
  inline vk::Instance getInstance() const { return mInstance.get(); }
  inline vk::Device getDevice() const { return mDevice.get(); }
  inline vk::PhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
  inline vk::CommandPool getCommandPool() const { return mCommandPool.get(); }
  inline vk::DescriptorPool getDescriptorPool() const { return mDescriptorPool.get(); }

private:
#ifdef VK_VALIDATION
  bool checkValidationLayerSupport();
#endif

  /** Create Vulkan instance */
  void createInstance();

  /** Choose physical device */
  void pickPhysicalDevice();

  /** Create logical device */
  void createLogicalDevice();

  /** Create command pool for rendering */
  void createCommandPool();

  /** Create descriptor pool rendering */
  void createDescriptorPool();

private:
  struct DescriptorSetLayouts {
    vk::UniqueDescriptorSetLayout scene;
    vk::UniqueDescriptorSetLayout camera;
    vk::UniqueDescriptorSetLayout material;
    vk::UniqueDescriptorSetLayout object;
  } mDescriptorSetLayouts;

private:
  std::shared_ptr<VulkanMesh> mCubeMesh{};
  std::shared_ptr<VulkanMesh> mSphereMesh{};
  std::shared_ptr<VulkanMesh> mYZPlaneMesh{};

public:
  void initializeDescriptorSetLayouts();
  inline DescriptorSetLayouts const &getDescriptorSetLayouts() const {
    return mDescriptorSetLayouts;
  }

  std::unique_ptr<Object> createObject(std::shared_ptr<VulkanMesh> mesh,
                                       std::shared_ptr<VulkanMaterial> material);

  std::vector<std::unique_ptr<Object>> loadObjects(std::string const &file,
                                                   glm::vec3 scale = {1.f, 1.f, 1.f});
  std::unique_ptr<Object> loadSphere();
  std::unique_ptr<Object> loadCube();
  std::unique_ptr<Object> loadCapsule(float radius, float halfLength);
  std::unique_ptr<Object> loadYZPlane();

  std::shared_ptr<class VulkanMaterial> createMaterial();
  std::unique_ptr<class VulkanScene> createVulkanScene() const;
  std::unique_ptr<struct VulkanObject> createVulkanObject() const;
  std::unique_ptr<VulkanRenderer> createVulkanRenderer(VulkanRendererConfig const &config = {});
  std::unique_ptr<VulkanRendererForEditor>
  createVulkanRendererForEditor(VulkanRendererConfig const &config = {});
  std::unique_ptr<struct Camera> createCamera() const;

  std::shared_ptr<struct VulkanTextureData> loadTexture(std::string const &filename);
  std::shared_ptr<struct VulkanTextureData> getPlaceholderTexture();

#ifdef ON_SCREEN
  std::unique_ptr<VulkanWindow> createWindow(uint32_t width = 800, uint32_t height = 600);
#endif

public:
  static std::string gDefaultShaderDir;
  static void setDefaultShaderDir(std::string const &dir);
};

} // namespace svulkan
