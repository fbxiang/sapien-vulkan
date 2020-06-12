#pragma once

#ifdef ON_SCREEN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#endif

#include "common/glm_common.h"

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

#ifdef ON_SCREEN
  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  vk::UniqueSurfaceKHR mSurface;
  GLFWwindow *mWindow;
#endif

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
  } mDescriptorSetLayouts;

  
 public:
  void initializeDescriptorSetLayouts();
  inline DescriptorSetLayouts const& getDescriptorSetLayouts() const { return mDescriptorSetLayouts; }

  std::vector<std::unique_ptr<Object>> loadObjects(std::string const &file, float scale,
                                                         bool ignoreRootTransform,
                                                         glm::vec3 const &up, glm::vec3 const &forward);

  std::shared_ptr<struct VulkanMaterial> createMaterial() const;
  std::unique_ptr<class VulkanScene> createVulkanScene() const;
  std::unique_ptr<struct VulkanObject> createVulkanObject() const;
  std::unique_ptr<VulkanRenderer> createVulkanRenderer();
  std::unique_ptr<struct Camera> createCamera() const;
};

}
