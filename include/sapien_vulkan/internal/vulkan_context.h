#pragma once

#ifdef ON_SCREEN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include "sapien_vulkan/common/glm_common.h"

namespace svulkan
{

class Object;
class VulkanRenderer;

class VulkanContext {

 private:
  vk::PhysicalDevice mPhysicalDevice;
  vk::UniqueInstance mInstance;
  vk::UniqueDevice mDevice;
  vk::UniqueCommandPool mCommandPool;
  vk::UniqueDescriptorPool mDescriptorPool;

  uint32_t graphicsQueueFamilyIndex;
#ifdef ON_SCREEN
  uint32_t presentQueueFamilyIndex;
  vk::UniqueSurfaceKHR mSurface;
  GLFWwindow *mWindow;
#endif

  std::shared_ptr<struct VulkanTextureData> mPlaceholderTexture {nullptr};

 public:
  VulkanContext();

  /** Get the graphics queue */
  vk::Queue getGraphicsQueue() const;

#ifdef ON_SCREEN
  vk::Queue getPresentQueue() const;
#endif

  inline uint32_t getGraphicsQueueFamilyIndex() { return graphicsQueueFamilyIndex; }
  inline vk::Instance getInstance() const { return mInstance.get(); }
  inline vk::Device getDevice() const { return mDevice.get(); }
  inline vk::PhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
  inline vk::CommandPool getCommandPool() const { return mCommandPool.get(); }
  inline vk::DescriptorPool getDescriptorPool() const { return mDescriptorPool.get(); }

#ifdef ON_SCREEN
  inline uint32_t getPresentQueueFamilyIndex() { return presentQueueFamilyIndex; }
  inline GLFWwindow *getWindow() const { return mWindow; }
  inline vk::SurfaceKHR getSurface() const { return mSurface.get(); }
#endif

 private:
#ifdef VK_VALIDATION
  bool checkValidationLayerSupport();
#endif

#ifdef ON_SCREEN
  void createWindow();
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

    vk::UniqueDescriptorSetLayout deferred;
  } mDescriptorSetLayouts;

  
 public:
  void initializeDescriptorSetLayouts();
  inline DescriptorSetLayouts const& getDescriptorSetLayouts() const { return mDescriptorSetLayouts; }

  std::vector<std::unique_ptr<Object>> loadObjects(std::string const &file,
                                                   glm::vec3 scale = {1.f, 1.f, 1.f},
                                                   bool ignoreRootTransform = true,
                                                   glm::vec3 const &up = {0,1,0},
                                                   glm::vec3 const &forward = {0,0, -1});

  std::unique_ptr<Object> loadSphere();
  std::unique_ptr<Object> loadCube();
  std::unique_ptr<Object> loadCapsule(float radius, float halfLength);
  std::unique_ptr<Object> loadYZPlane();

  std::shared_ptr<class VulkanMaterial> createMaterial();
  std::unique_ptr<class VulkanScene> createVulkanScene() const;
  std::unique_ptr<struct VulkanObject> createVulkanObject() const;
  std::unique_ptr<VulkanRenderer> createVulkanRenderer();
  std::unique_ptr<struct Camera> createCamera() const;

  std::shared_ptr<struct VulkanTextureData> loadTexture(std::string const &filename) const;
  std::shared_ptr<struct VulkanTextureData> getPlaceholderTexture();
};

}
