#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/camera.h"
#include "sapien_vulkan/data/geometry.hpp"
#include "sapien_vulkan/internal/vulkan.h"
#include "sapien_vulkan/internal/vulkan_material.h"
#include "sapien_vulkan/internal/vulkan_object.h"
#include "sapien_vulkan/internal/vulkan_renderer.h"
#include "sapien_vulkan/internal/vulkan_renderer_for_editor.h"
#include "sapien_vulkan/internal/vulkan_scene.h"
#include "sapien_vulkan/internal/vulkan_texture.h"
#include "sapien_vulkan/object.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>

#include <vulkan/vulkan_beta.h>

namespace svulkan {

static void glfwErrorCallback(int error_code, const char *description) {
  log::error("GLFW error: {}", description);
}

VulkanContext::VulkanContext(bool requirePresent, uint32_t objectBufferSize)
    : mRequirePresent(requirePresent), mObjectBufferSize(objectBufferSize),
      mResourcesManager(*this) {
  createInstance();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandPool();
  createDescriptorPool();

  initializeDescriptorSetLayouts();
}

#ifdef VK_VALIDATION
bool VulkanContext::checkValidationLayerSupport() {
  static const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  return true;
}
#endif

void VulkanContext::createInstance() {

#ifdef ON_SCREEN
  if (mRequirePresent) {
    log::info("Initializing GLFW");
    glfwInit();
    glfwSetErrorCallback(glfwErrorCallback);
  }
#endif

#ifdef VK_VALIDATION
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error(
        "create instance failed: validation layers requested but not available");
  }
  std::vector<const char *> enabledLayers = {"VK_LAYER_KHRONOS_validation"};
#else
  std::vector<const char *> enabledLayers = {};
#endif

  vk::ApplicationInfo appInfo("Vulkan Renderer", VK_MAKE_VERSION(0, 0, 1), "No Engine",
                              VK_MAKE_VERSION(0, 0, 1), VK_API_VERSION_1_1);

  auto extensionProperties = vk::enumerateInstanceExtensionProperties(nullptr);
  std::string allExtensions;
  for (auto &p : extensionProperties) {
    allExtensions += std::string(p.extensionName) + " ";
  }
  spdlog::info("All available extensions: {}", allExtensions);

  std::vector<const char *> instanceExtensions;

#ifdef ON_SCREEN
  if (mRequirePresent) {
    if (!glfwVulkanSupported()) {
      throw std::runtime_error("Failed to create instance: GLFW does not support Vulkan");
    }
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensions) {
      throw std::runtime_error("Failed to create instance: cannot get required GLFW extensions");
    }

    std::string extensionNames = "";
    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
      extensionNames += std::string(glfwExtensions[i]) + " ";
    }
    log::info("GLFW requested extensions: {}", extensionNames);

    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
      instanceExtensions.push_back(glfwExtensions[i]);
    }
  }
#endif

  vk::InstanceCreateInfo createInfo({}, &appInfo, enabledLayers.size(), enabledLayers.data(),
                                    instanceExtensions.size(), instanceExtensions.data());

  mInstance = vk::createInstanceUnique(createInfo);
}

void VulkanContext::pickPhysicalDevice() {
  GLFWwindow *window;
  VkSurfaceKHR tmpSurface;

  if (mRequirePresent) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(1, 1, "vulkan", nullptr, nullptr);
    auto result = glfwCreateWindowSurface(mInstance.get(), window, nullptr, &tmpSurface);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("create window failed: glfwCreateWindowSurface failed");
    }
  }

  vk::PhysicalDevice pickedDevice;
  uint32_t pickedIndex;
  for (auto device : mInstance->enumeratePhysicalDevices()) {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        device.getQueueFamilyProperties();
    pickedIndex = queueFamilyProperties.size();
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
      if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
        if (mRequirePresent && !device.getSurfaceSupportKHR(i, tmpSurface)) {
          continue;
        }
        pickedIndex = i;
      }
    }
    if (pickedIndex == queueFamilyProperties.size()) {
      continue;
    }
    auto features = device.getFeatures();
    if (!features.independentBlend) {
      continue;
    }
    pickedDevice = device;
    break;
  }
  if (!pickedDevice) {
    throw std::runtime_error("pickPhysicalDevice: no compatible device found.");
  }
  mPhysicalDevice = pickedDevice;
  graphicsQueueFamilyIndex = pickedIndex;
  if (mRequirePresent) {
    vkDestroySurfaceKHR(mInstance.get(), tmpSurface, nullptr);
    glfwDestroyWindow(window);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  }
}

void VulkanContext::createLogicalDevice() {
  float queuePriority = 0.0f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(),
                                                  static_cast<uint32_t>(graphicsQueueFamilyIndex),
                                                  1, &queuePriority);
  std::vector<const char *> deviceExtensions{};
  vk::PhysicalDeviceFeatures features;
  features.independentBlend = true;

#ifdef ON_SCREEN
  if (mRequirePresent) {
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }
#endif
  mDevice = mPhysicalDevice.createDeviceUnique(
      vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo, 0, nullptr,
                           deviceExtensions.size(), deviceExtensions.data(), &features));
}

vk::Queue VulkanContext::getGraphicsQueue() const {
  return mDevice->getQueue(graphicsQueueFamilyIndex, 0);
}

void VulkanContext::createCommandPool() {
  mCommandPool = mDevice->createCommandPoolUnique(vk::CommandPoolCreateInfo(
      vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamilyIndex));
}

void VulkanContext::createDescriptorPool() {
  mDescriptorPool = svulkan::createDescriptorPool(
      mDevice.get(), {{vk::DescriptorType::eSampler, 1000},
                      {vk::DescriptorType::eCombinedImageSampler, 1000},
                      {vk::DescriptorType::eSampledImage, 1000},
                      {vk::DescriptorType::eStorageImage, 1000},
                      {vk::DescriptorType::eUniformTexelBuffer, 1000},
                      {vk::DescriptorType::eStorageTexelBuffer, 1000},
                      {vk::DescriptorType::eUniformBuffer, 1000 + mObjectBufferSize},
                      {vk::DescriptorType::eStorageBuffer, 1000},
                      {vk::DescriptorType::eUniformBufferDynamic, 1000},
                      {vk::DescriptorType::eStorageBufferDynamic, 1000},
                      {vk::DescriptorType::eInputAttachment, 1000}});
}

void VulkanContext::initializeDescriptorSetLayouts() {
  // uniform buffers
  mDescriptorSetLayouts.scene = createDescriptorSetLayout(
      getDevice(), {{vk::DescriptorType::eUniformBuffer, 1,
                     vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}});

  mDescriptorSetLayouts.camera = createDescriptorSetLayout(
      getDevice(), {{vk::DescriptorType::eUniformBuffer, 1,
                     vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}});

  mDescriptorSetLayouts.object = createDescriptorSetLayout(
      getDevice(), {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});

  mDescriptorSetLayouts.material = createDescriptorSetLayout(
      getDevice(),
      {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment},
       {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
       {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
       {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
       {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}});
}

std::shared_ptr<VulkanTextureData> VulkanContext::getPlaceholderTexture() {
  return mResourcesManager.getPlaceholderTexture();
}

std::shared_ptr<struct VulkanTextureData> VulkanContext::loadTexture(std::string const &filename) {
  return mResourcesManager.loadTexture(filename);
}

std::unique_ptr<Object> VulkanContext::createObject(std::shared_ptr<VulkanMesh> mesh,
                                                    std::shared_ptr<VulkanMaterial> material) {
  std::unique_ptr<VulkanObject> vobj = std::make_unique<VulkanObject>(
      getPhysicalDevice(), getDevice(), mDescriptorPool.get(), mDescriptorSetLayouts.object.get());
  vobj->setMesh(mesh);
  vobj->setMaterial(material);
  return std::make_unique<Object>(std::move(vobj));
}

std::unique_ptr<Object> VulkanContext::loadCube() {
  return createObject(mResourcesManager.loadCube(), createMaterial());
}

std::unique_ptr<Object> VulkanContext::loadSphere() {
  return createObject(mResourcesManager.loadSphere(), createMaterial());
}

std::unique_ptr<Object> VulkanContext::loadCapsule(float radius, float halfLength) {
  return createObject(mResourcesManager.loadCapsule(radius, halfLength), createMaterial());
}

std::unique_ptr<Object> VulkanContext::loadYZPlane() {
  return createObject(mResourcesManager.loadYZPlane(), createMaterial());
}

std::vector<std::unique_ptr<Object>> VulkanContext::loadObjects(std::string const &file,
                                                                glm::vec3 scale) {
  std::vector<std::unique_ptr<Object>> results;
  auto meshMat = mResourcesManager.loadFile(file);
  for (auto [mesh, mat] : meshMat) {
    results.push_back(createObject(mesh, mat));
    results.back()->mTransform.scale = scale;
  }
  return results;
}

std::shared_ptr<VulkanMaterial> VulkanContext::createMaterial() {
  auto mat = std::make_shared<VulkanMaterial>(
      getPhysicalDevice(), getDevice(), mDescriptorPool.get(),
      mDescriptorSetLayouts.material.get(), getPlaceholderTexture());
  mat->setProperties({});
  return mat;
}

std::unique_ptr<VulkanScene> VulkanContext::createVulkanScene() const {
  return std::make_unique<VulkanScene>(getPhysicalDevice(), getDevice(), getDescriptorPool(),
                                       getDescriptorSetLayouts().scene.get());
}

std::unique_ptr<VulkanObject> VulkanContext::createVulkanObject() const {
  return std::make_unique<VulkanObject>(getPhysicalDevice(), getDevice(), getDescriptorPool(),
                                        getDescriptorSetLayouts().object.get());
}

std::unique_ptr<VulkanRenderer>
VulkanContext::createVulkanRenderer(VulkanRendererConfig const &config) {
  return std::make_unique<VulkanRenderer>(*this, config);
}

std::unique_ptr<VulkanRendererForEditor>
VulkanContext::createVulkanRendererForEditor(VulkanRendererConfig const &config) {
  return std::make_unique<VulkanRendererForEditor>(*this, config);
}

std::unique_ptr<struct Camera> VulkanContext::createCamera() const {
  return std::make_unique<Camera>(getPhysicalDevice(), getDevice(), getDescriptorPool(),
                                  getDescriptorSetLayouts().camera.get());
}

VulkanContext::~VulkanContext() {
#ifdef ON_SCREEN
  if (mRequirePresent) {
    glfwTerminate();
  }
#endif
}

#ifdef ON_SCREEN
std::unique_ptr<VulkanWindow> VulkanContext::createWindow(uint32_t width, uint32_t height) {
  return std::make_unique<VulkanWindow>(
      getInstance(), getDevice(), getPhysicalDevice(), getGraphicsQueueFamilyIndex(),
      std::vector{vk::Format::eB8G8R8A8Unorm, vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8Unorm,
                  vk::Format::eR8G8B8Unorm},
      vk::ColorSpaceKHR::eSrgbNonlinear, width, height);
}
#endif

// static members

std::string VulkanContext::gDefaultShaderDir{"spv"};
void VulkanContext::setDefaultShaderDir(std::string const &dir) { gDefaultShaderDir = dir; }

} // namespace svulkan
