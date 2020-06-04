#include "vulkan_context.h"

namespace svulkan
{

VulkanContext::VulkanContext() {
  createInstance();
  pickPhysicalDevice();
  createLogicalDevice();
#ifdef ON_SCREEN
  createWindow();
#endif
  createCommandPool();
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
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(800, 800, "vulkan", nullptr, nullptr);
#endif

#ifdef VK_VALIDATION
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
  }
  std::vector<const char *> enabledLayers = {"VK_LAYER_KHRONOS_validation"};
#else
  std::vector<const char *> enabledLayers = {};
#endif

  vk::ApplicationInfo appInfo("Vulkan Renderer", VK_MAKE_VERSION(0, 0, 1), "No Engine",
                              VK_MAKE_VERSION(0, 0, 1), VK_API_VERSION_1_1);

  std::vector<const char *> instanceExtensions;

#ifdef ON_SCREEN
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
    instanceExtensions.push_back(glfwExtensions[i]);
  }
#endif

  vk::InstanceCreateInfo createInfo({}, &appInfo, enabledLayers.size(), enabledLayers.data(),
                                    instanceExtensions.size(), instanceExtensions.data());

  mInstance = vk::createInstanceUnique(createInfo);
}

void VulkanContext::pickPhysicalDevice() { mPhysicalDevice = mInstance->enumeratePhysicalDevices().front(); }

void VulkanContext::createLogicalDevice() {
  std::vector<vk::QueueFamilyProperties> queueFamilyProperties = mPhysicalDevice.getQueueFamilyProperties();
  graphicsQueueFamilyIndex = std::distance(
      queueFamilyProperties.begin(), std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                                  [](vk::QueueFamilyProperties const &qfp) {
                                                    return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                                                  }));
  assert(graphicsQueueFamilyIndex < queueFamilyProperties.size());

  float queuePriority = 0.0f;
  vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
      vk::DeviceQueueCreateFlags(), static_cast<uint32_t>(graphicsQueueFamilyIndex), 1, &queuePriority);
  std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  mDevice = mPhysicalDevice.createDeviceUnique(
      vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo, 0, nullptr,
                           deviceExtensions.size(), deviceExtensions.data()));
}

#ifdef ON_SCREEN
void VulkanContext::createWindow() {
  VkSurfaceKHR tmpSurface;
  if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &tmpSurface) != VK_SUCCESS) {
    throw std::runtime_error("Create window failed: cannot create GLFW window surface");
  }
  mSurface = vk::UniqueSurfaceKHR(tmpSurface);

  if (!mPhysicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, mSurface.get())) {
    throw std::runtime_error("Create window failed: graphics device does not have present capability");
  }

  presentQueueFamilyIndex = graphicsQueueFamilyIndex;
}
#endif

vk::Queue VulkanContext::getGraphicsQueue() const { return mDevice->getQueue(graphicsQueueFamilyIndex, 0); }

#ifdef ON_SCREEN
vk::Queue VulkanContext::getPresentQueue() const { return mDevice->getQueue(presentQueueFamilyIndex, 0); }
#endif

}
