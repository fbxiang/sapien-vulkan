#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/internal/vulkan.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "sapien_vulkan/internal/vulkan_material.h"
#include "sapien_vulkan/internal/vulkan_object.h"
#include "sapien_vulkan/object.h"
#include "sapien_vulkan/internal/vulkan_scene.h"
#include <iostream>
#include "sapien_vulkan/internal/vulkan_renderer.h"
#include "sapien_vulkan/pass/gbuffer.h"
#include "sapien_vulkan/pass/deferred.h"
#include "sapien_vulkan/camera.h"
#include "sapien_vulkan/internal/vulkan_texture.h"
#include "sapien_vulkan/data/geometry.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "sapien_vulkan/common/stb_image.h"

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
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mWindow = glfwCreateWindow(800, 600, "vulkan", nullptr, nullptr);
#endif

#ifdef VK_VALIDATION
  if (!checkValidationLayerSupport()) {
    throw std::runtime_error("create instance failed: validation layers requested but not available");
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
#ifdef ON_SCREEN
  std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#else
  std::vector<const char *> deviceExtensions = {};
#endif
  mDevice = mPhysicalDevice.createDeviceUnique(
      vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo, 0, nullptr,
                           deviceExtensions.size(), deviceExtensions.data()));
}

#ifdef ON_SCREEN
void VulkanContext::createWindow() {
  VkSurfaceKHR tmpSurface;
  if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &tmpSurface) != VK_SUCCESS) {
    throw std::runtime_error("create window failed: cannot create GLFW window surface");
  }
  mSurface = vk::UniqueSurfaceKHR(tmpSurface);

  if (!mPhysicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, mSurface.get())) {
    throw std::runtime_error("create window failed: graphics device does not have present capability");
  }

  presentQueueFamilyIndex = graphicsQueueFamilyIndex;
}
#endif

vk::Queue VulkanContext::getGraphicsQueue() const { return mDevice->getQueue(graphicsQueueFamilyIndex, 0); }

#ifdef ON_SCREEN
vk::Queue VulkanContext::getPresentQueue() const { return mDevice->getQueue(presentQueueFamilyIndex, 0); }
#endif

void VulkanContext::createCommandPool() {
  mCommandPool = mDevice->createCommandPoolUnique(
      vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsQueueFamilyIndex));
}

void VulkanContext::createDescriptorPool() {
  // constexpr uint32_t numMaterials = 100;
  // constexpr uint32_t numTextures = 100;
  mDescriptorPool = svulkan::createDescriptorPool(mDevice.get(), {
      // {vk::DescriptorType::eUniformBuffer, numMaterials},
      // {vk::DescriptorType::eCombinedImageSampler, numTextures},
      { vk::DescriptorType::eSampler, 1000 },
      { vk::DescriptorType::eCombinedImageSampler, 1000 },
      { vk::DescriptorType::eSampledImage, 1000 },
      { vk::DescriptorType::eStorageImage, 1000 },
      { vk::DescriptorType::eUniformTexelBuffer, 1000 },
      { vk::DescriptorType::eStorageTexelBuffer, 1000 },
      { vk::DescriptorType::eUniformBuffer, 1000 },
      { vk::DescriptorType::eStorageBuffer, 1000 },
      { vk::DescriptorType::eUniformBufferDynamic, 1000 },
      { vk::DescriptorType::eStorageBufferDynamic, 1000 },
      { vk::DescriptorType::eInputAttachment, 1000 }
    });
}

void VulkanContext::initializeDescriptorSetLayouts() {
  // uniform buffers
  mDescriptorSetLayouts.scene = 
      createDescriptorSetLayout(getDevice(), {{vk::DescriptorType::eUniformBuffer,
            1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}});

  mDescriptorSetLayouts.camera = 
      createDescriptorSetLayout(getDevice(), {{vk::DescriptorType::eUniformBuffer,
            1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment}});

  mDescriptorSetLayouts.object = 
      createDescriptorSetLayout(getDevice(), {{vk::DescriptorType::eUniformBuffer,
            1, vk::ShaderStageFlagBits::eVertex}});

  mDescriptorSetLayouts.material = 
      createDescriptorSetLayout(
          getDevice(),
          {{vk::DescriptorType::eUniformBuffer, 1,  vk::ShaderStageFlagBits::eFragment},
           {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
           {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
           {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},
           {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
          });

  mDescriptorSetLayouts.deferred = createDescriptorSetLayout(
      getDevice(), {
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},  // albedo
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},  // position
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},  // specular
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment},  // normal
        {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}  // depth
      });
}

std::shared_ptr<VulkanTextureData> VulkanContext::getPlaceholderTexture() {
  if (!mPlaceholderTexture) {
    mPlaceholderTexture = std::make_shared<VulkanTextureData>(
        getPhysicalDevice(), getDevice(), vk::Extent2D{1u, 1u});
    mPlaceholderTexture->setImage(getPhysicalDevice(), getDevice(), getCommandPool(), getGraphicsQueue(),
                                  [&](void *target, vk::Extent2D const &extent) {});
  }
  return mPlaceholderTexture;
}

std::shared_ptr<struct VulkanTextureData> VulkanContext::loadTexture(std::string const &filename) const {
  int width, height, nrChannels;
  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
  auto texture = std::make_shared<VulkanTextureData>(
      getPhysicalDevice(), getDevice(),
      vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)});

  texture->setImage(getPhysicalDevice(), getDevice(), getCommandPool(), getGraphicsQueue(),
                    [&](void *target, vk::Extent2D const &extent) {
                      memcpy(target, data, extent.width * extent.height * 4);
                    });
  stbi_image_free(data);
  return texture;
}

static float shininessToRoughness(float ns) {
  if (ns <= 5.f) {
    return 1.f;
  }
  if (ns >= 1605.f) {
    return 0.f;
  }
  return 1.f - (std::sqrt(ns - 5.f) * 0.025f);
}

std::unique_ptr<Object> VulkanContext::loadCube(glm::vec3 halfExtens) {
  std::vector vertices = FlatCubeVertices;
  std::vector indices = FlatCubeIndices;
  static std::shared_ptr<VulkanMesh> vulkanMesh = std::make_shared<VulkanMesh>(
      getPhysicalDevice(), getDevice(), mCommandPool.get(),
      getGraphicsQueue(), vertices, indices, false);

  std::unique_ptr<VulkanObject> vobj =
      std::make_unique<VulkanObject>(getPhysicalDevice(), getDevice(),
                                     mDescriptorPool.get(), mDescriptorSetLayouts.object.get());
  vobj->setMesh(vulkanMesh);
  std::shared_ptr<VulkanMaterial> mat = createMaterial();
  vobj->setMaterial(mat);

  auto obj = std::make_unique<Object>(std::move(vobj));
  obj->mTransform.scale = halfExtens;
  return obj;
}

std::unique_ptr<Object> VulkanContext::loadSphere(float radius) {
  static std::shared_ptr<VulkanMesh> vulkanMesh {nullptr};
  if (!vulkanMesh) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t stacks = 20;
    uint32_t slices = 20;

    for (uint32_t i = 0; i <= stacks; ++i) {
      float phi = glm::pi<float>() / stacks * i - glm::pi<float>() / 2;
      for (uint32_t j = 0; j < slices; ++j) {
        float theta = glm::pi<float>() * 2 / slices * j;
        float x = sinf(phi);
        float y = cosf(theta) * cosf(phi);
        float z = sinf(theta) * cosf(phi);
        vertices.push_back({{x, y, z}, {x, y, z}});
      }
    }

    for (uint32_t i = 0; i < stacks * slices; ++i) {
      uint32_t right = (i + 1) % slices + i / slices * slices;
      uint32_t up = i + slices;
      uint32_t rightUp = right + slices;

      if (i >= slices) {
        indices.push_back(i);
        indices.push_back(right);
        indices.push_back(rightUp);
      }

      if (i < (stacks-1) * slices) {
        indices.push_back(i);
        indices.push_back(rightUp);
        indices.push_back(up);
      }
    }

    vulkanMesh = std::make_shared<VulkanMesh>(getPhysicalDevice(), getDevice(), mCommandPool.get(),
                                              getGraphicsQueue(), vertices, indices, false);
  }

  std::unique_ptr<VulkanObject> vobj =
      std::make_unique<VulkanObject>(getPhysicalDevice(), getDevice(),
                                     mDescriptorPool.get(), mDescriptorSetLayouts.object.get());
  vobj->setMesh(vulkanMesh);
  std::shared_ptr<VulkanMaterial> mat = createMaterial();
  vobj->setMaterial(mat);

  auto obj = std::make_unique<Object>(std::move(vobj));
  obj->mTransform.scale = {radius, radius, radius};
  return obj;
}

// std::unique_ptr<Object> VulkanContext::loadCapsule(float radius, float halfLength) {
// }

std::unique_ptr<Object> VulkanContext::loadYZPlane(glm::vec2 halfExtents) {
  static std::shared_ptr<VulkanMesh> vulkanMesh {nullptr};

  if (!vulkanMesh)
  {
    std::vector<Vertex> vertices = {{{0, -1, -1}, {1, 0, 0}, {0, 0}, {0, 1, 0}, {0, 0, 1}},
                                    {{0, 1, -1}, {1, 0, 0}, {1, 0}, {0, 1, 0}, {0, 0, 1}},
                                    {{0, -1, 1}, {1, 0, 0}, {0, 1}, {0, 1, 0}, {0, 0, 1}},
                                    {{0, 1, 1}, {1, 0, 0}, {1, 1}, {0, 1, 0}, {0, 0, 1}}};
    std::vector<uint32_t> indices = { 0, 1, 3, 0, 3, 2 };

  vulkanMesh = std::make_shared<VulkanMesh>(getPhysicalDevice(), getDevice(), mCommandPool.get(),
                                            getGraphicsQueue(), vertices, indices, false);
  }

  std::unique_ptr<VulkanObject> vobj =
      std::make_unique<VulkanObject>(getPhysicalDevice(), getDevice(),
                                     mDescriptorPool.get(), mDescriptorSetLayouts.object.get());
  vobj->setMesh(vulkanMesh);
  std::shared_ptr<VulkanMaterial> mat = createMaterial();
  vobj->setMaterial(mat);

  auto obj = std::make_unique<Object>(std::move(vobj));
  obj->mTransform.scale = glm::vec3{1, halfExtents};
  return obj;
}

std::vector<std::unique_ptr<Object>> VulkanContext::loadObjects(std::string const &file,
                                                                glm::vec3 scale,
                                                                bool ignoreRootTransform,
                                                                glm::vec3 const &up,
                                                                glm::vec3 const &forward) {

  glm::mat3 formatTransform = glm::mat3(glm::cross(forward, up), up, -forward);
  std::vector<std::unique_ptr<Object>> objects;
  Assimp::Importer importer;
  uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                   aiProcess_GenNormals | aiProcess_FlipUVs | aiProcess_PreTransformVertices;
  const aiScene *scene = importer.ReadFile(file, flags);
  if (scene->mRootNode->mMetaData) {
    throw std::runtime_error("Failed to load scene: metadata is not supported, " + file);
  }
  if (!scene) {
    throw std::runtime_error("Failed to load scene: " + std::string(importer.GetErrorString()) + ", " + file);
  }

  // printf("Loaded %d meshes, %d materials, %d textures\n", scene->mNumMeshes, scene->mNumMaterials,
  //        scene->mNumTextures);

  std::vector<std::shared_ptr<VulkanMaterial>> mats;
  for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
    std::shared_ptr<VulkanMaterial> mat = createMaterial();
    auto *m = scene->mMaterials[i];
    PBRMaterialUBO matSpec;

    aiColor3D color{0,0,0};
    float alpha = 1.f;
    float shininess = 0.f;
    m->Get(AI_MATKEY_OPACITY, alpha);
    m->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    matSpec.baseColor = {color.r, color.g, color.b, alpha};

    m->Get(AI_MATKEY_COLOR_SPECULAR, color);
    matSpec.specular = (color.r + color.g + color.b) / 3.f;

    m->Get(AI_MATKEY_SHININESS, shininess);
    matSpec.roughness = shininessToRoughness(shininess);

    std::string parentdir = file.substr(0, file.find_last_of('/')) + "/";

    aiString path;
    if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
        m->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
      std::string p = std::string(path.C_Str());
      std::string fullPath = parentdir + p;

      mat->setDiffuseTexture(loadTexture(fullPath));
      matSpec.hasColorMap = 1;
      log::info("Color texture loaded: {}", fullPath);
    }

    if (m->GetTextureCount(aiTextureType_SPECULAR) > 0 &&
        m->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS) {
      std::string p = std::string(path.C_Str());
      std::string fullPath = parentdir + p;

      mat->setSpecularTexture(loadTexture(fullPath));
      matSpec.hasSpecularMap = 1;
      log::info("Specular texture loaded: {}", fullPath);
    }

    if (m->GetTextureCount(aiTextureType_NORMALS) > 0 &&
        m->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
      std::string p = std::string(path.C_Str());
      std::string fullPath = parentdir + p;

      mat->setNormalTexture(loadTexture(fullPath));
      matSpec.hasNormalMap = 1;
      log::info("Normal texture loaded: {}", fullPath);
    }

    if (m->GetTextureCount(aiTextureType_HEIGHT) > 0 &&
        m->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS) {
      std::string p = std::string(path.C_Str());
      std::string fullPath = parentdir + p;

      mat->setHeightTexture(loadTexture(fullPath));
      matSpec.hasHeightMap = 1;
      log::info("Height texture loaded: {}", fullPath);
    }
    mat->setProperties(matSpec);
    mats.push_back(mat);
  }

  for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    auto mesh = scene->mMeshes[i];
    if (!mesh->HasFaces())
      continue;

    for (uint32_t v = 0; v < mesh->mNumVertices; v++) {
      glm::vec3 normal = glm::vec3(0);
      glm::vec2 texcoord = glm::vec2(0);
      glm::vec3 position = formatTransform *
                           glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
      glm::vec3 tangent = glm::vec3(0);
      glm::vec3 bitangent = glm::vec3(0);
      if (mesh->HasNormals()) {
        normal = formatTransform * glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
      }
      if (mesh->HasTextureCoords(0)) {
        texcoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
      }
      if (mesh->HasTangentsAndBitangents()) {
        tangent =
            formatTransform * glm::vec3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
        bitangent = formatTransform *
                    glm::vec3(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
      }

      // FIXME: remove
      // printf("{{%.3ff,%.3ff,%.3ff}, {%.3ff,%.3ff,%.3ff}, {%.3ff,%.3ff}, {%.3ff,%.3ff,%.3ff}, {%.3ff,%.3ff,%.3ff}},\n",
      //        position.x, position.y, position.z,
      //        normal.x, normal.y, normal.z,
      //        texcoord.x, texcoord.y,
      //        tangent.x, tangent.y, tangent.z,
      //        bitangent.x, bitangent.y, bitangent.z);

      vertices.push_back({position, normal, texcoord, tangent, bitangent});
    }
    for (uint32_t f = 0; f < mesh->mNumFaces; f++) {
      auto face = mesh->mFaces[f];
      if (face.mNumIndices != 3) {
        fprintf(stderr, "A face with %d indices is found and ignored.", face.mNumIndices);
        continue;
      }
      indices.push_back(face.mIndices[0]);
      indices.push_back(face.mIndices[1]);
      indices.push_back(face.mIndices[2]);
    }
    
    // FIXME: remove
    // for (uint32_t i = 0; i < indices.size(); ++i) {
    //   printf("%d,", indices[i]);
    // }
    // printf("\n");

    std::shared_ptr<VulkanMesh> vulkanMesh = std::make_shared<VulkanMesh>(
        getPhysicalDevice(), getDevice(), mCommandPool.get(),
        getGraphicsQueue(), vertices, indices, !mesh->HasNormals());

    std::unique_ptr<VulkanObject> vobj =
        std::make_unique<VulkanObject>(getPhysicalDevice(), getDevice(),
                                       mDescriptorPool.get(), mDescriptorSetLayouts.object.get());
    vobj->setMesh(vulkanMesh);
    vobj->setMaterial(mats[mesh->mMaterialIndex]);
    std::unique_ptr<Object> object = std::make_unique<Object>(std::move(vobj));
    object->mTransform.scale = scale;
    objects.push_back(std::move(object));
  }
  return objects;
}

std::shared_ptr<VulkanMaterial> VulkanContext::createMaterial() {
  auto mat = std::make_shared<VulkanMaterial>(
      getPhysicalDevice(), getDevice(), mDescriptorPool.get(), mDescriptorSetLayouts.material.get(),
      getPlaceholderTexture());
  mat->setProperties({});
  return mat;
}

std::unique_ptr<VulkanScene> VulkanContext::createVulkanScene() const {
  return std::make_unique<VulkanScene>(
      getPhysicalDevice(), getDevice(), getDescriptorPool(), getDescriptorSetLayouts().scene.get());
}

std::unique_ptr<VulkanObject> VulkanContext::createVulkanObject() const {
  return std::make_unique<VulkanObject>(
      getPhysicalDevice(), getDevice(), getDescriptorPool(), getDescriptorSetLayouts().object.get());
}

std::unique_ptr<VulkanRenderer> VulkanContext::createVulkanRenderer() {
  return std::make_unique<VulkanRenderer>(*this);
}

std::unique_ptr<struct Camera> VulkanContext::createCamera() const {
  return std::make_unique<Camera>(
      getPhysicalDevice(), getDevice(), getDescriptorPool(), getDescriptorSetLayouts().camera.get());
}

}
