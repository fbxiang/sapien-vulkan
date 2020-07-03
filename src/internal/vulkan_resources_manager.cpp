#include "sapien_vulkan/internal/vulkan_resources_manager.h"
#include "sapien_vulkan/common/log.h"
#include "sapien_vulkan/data/geometry.hpp"
#include "sapien_vulkan/internal/vulkan_context.h"
#include "sapien_vulkan/internal/vulkan_material.h"
#include "sapien_vulkan/uniform_buffers.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <experimental/filesystem>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "sapien_vulkan/common/stb_image.h"

namespace svulkan {
namespace fs = std::experimental::filesystem;

static float shininessToRoughness(float ns) {
  if (ns <= 5.f) {
    return 1.f;
  }
  if (ns >= 1605.f) {
    return 0.f;
  }
  return 1.f - (std::sqrt(ns - 5.f) * 0.025f);
}

VulkanResourcesManager::VulkanResourcesManager(VulkanContext &context) : mContext(&context) {}

std::shared_ptr<VulkanMesh> VulkanResourcesManager::loadSphere() {
  if (!mSphereMesh) {
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

      if (i < (stacks - 1) * slices) {
        indices.push_back(i);
        indices.push_back(rightUp);
        indices.push_back(up);
      }
    }

    mSphereMesh = std::make_shared<VulkanMesh>(
        mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
        mContext->getGraphicsQueue(), vertices, indices, false);
  }
  return mSphereMesh;
}

std::shared_ptr<VulkanMesh> VulkanResourcesManager::loadCube() {
  if (!mCubeMesh) {
    std::vector vertices = FlatCubeVertices;
    std::vector indices = FlatCubeIndices;
    mCubeMesh = std::make_shared<VulkanMesh>(
        mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
        mContext->getGraphicsQueue(), vertices, indices, false);
  }
  return mCubeMesh;
}

std::shared_ptr<VulkanMesh> VulkanResourcesManager::loadCapsule(float radius, float halfLength) {
  std::shared_ptr<VulkanMesh> vulkanMesh{nullptr};
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  uint32_t stacks = 20;
  uint32_t slices = 20;

  // lower hemisphere
  for (uint32_t i = 0; i <= stacks / 2; ++i) {
    float phi = glm::pi<float>() / stacks * i - glm::pi<float>() / 2;
    for (uint32_t j = 0; j < slices; ++j) {
      float theta = glm::pi<float>() * 2 / slices * j;
      float x = sinf(phi);
      float y = cosf(theta) * cosf(phi);
      float z = sinf(theta) * cosf(phi);
      vertices.push_back({{radius * x - halfLength, radius * y, radius * z}, {x, y, z}});
    }
  }

  for (uint32_t i = 0; i < (stacks / 2) * slices; ++i) {
    uint32_t right = (i + 1) % slices + i / slices * slices;
    uint32_t up = i + slices;
    uint32_t rightUp = right + slices;

    if (i >= slices) {
      indices.push_back(i);
      indices.push_back(right);
      indices.push_back(rightUp);
    }

    indices.push_back(i);
    indices.push_back(rightUp);
    indices.push_back(up);
  }

  // upper hemisphere
  for (uint32_t i = stacks / 2; i <= stacks; ++i) {
    float phi = glm::pi<float>() / stacks * i - glm::pi<float>() / 2;
    for (uint32_t j = 0; j < slices; ++j) {
      float theta = glm::pi<float>() * 2 / slices * j;
      float x = sinf(phi);
      float y = cosf(theta) * cosf(phi);
      float z = sinf(theta) * cosf(phi);
      vertices.push_back({{radius * x + halfLength, radius * y, radius * z}, {x, y, z}});
    }
  }

  for (uint32_t i = (stacks / 2 + 1) * slices; i < (stacks + 1) * slices; ++i) {
    uint32_t right = (i + 1) % slices + i / slices * slices;
    uint32_t up = i + slices;
    uint32_t rightUp = right + slices;

    indices.push_back(i);
    indices.push_back(right);
    indices.push_back(rightUp);

    if (i < stacks * slices) {
      indices.push_back(i);
      indices.push_back(rightUp);
      indices.push_back(up);
    }
  }

  // cylinder
  uint32_t start = vertices.size() / 2 - slices, end = vertices.size() / 2 + slices;
  uint32_t idx = vertices.size();
  for (uint32_t i = start; i < end; ++i) {
    vertices.push_back(vertices[i]);
  }
  for (uint32_t i = idx; i < idx + slices; ++i) {
    uint32_t right = (i + 1) % slices + idx;
    uint32_t up = i + slices;
    uint32_t rightUp = right + slices;

    indices.push_back(i);
    indices.push_back(right);
    indices.push_back(rightUp);

    indices.push_back(i);
    indices.push_back(rightUp);
    indices.push_back(up);
  }

  return std::make_shared<VulkanMesh>(mContext->getPhysicalDevice(), mContext->getDevice(),
                                      mContext->getCommandPool(), mContext->getGraphicsQueue(),
                                      vertices, indices, false);
}

std::shared_ptr<VulkanMesh> VulkanResourcesManager::loadYZPlane() {
  if (!mYZPlaneMesh) {
    std::vector<Vertex> vertices = {{{0, -1, -1}, {1, 0, 0}, {0, 0}, {0, 1, 0}, {0, 0, 1}},
                                    {{0, 1, -1}, {1, 0, 0}, {1, 0}, {0, 1, 0}, {0, 0, 1}},
                                    {{0, -1, 1}, {1, 0, 0}, {0, 1}, {0, 1, 0}, {0, 0, 1}},
                                    {{0, 1, 1}, {1, 0, 0}, {1, 1}, {0, 1, 0}, {0, 0, 1}}};
    std::vector<uint32_t> indices = {0, 1, 3, 0, 3, 2};

    mYZPlaneMesh = std::make_shared<VulkanMesh>(
        mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
        mContext->getGraphicsQueue(), vertices, indices, false);
  }
  return mYZPlaneMesh;
}

std::vector<std::pair<std::shared_ptr<VulkanMesh>, std::shared_ptr<class VulkanMaterial>>>
VulkanResourcesManager::loadFile(std::string const &file) {
  std::string fullPath = fs::canonical(file);
  if (!fs::is_regular_file(fullPath)) {
    log::error("Mesh file not found: {}", fullPath);
    return {};
  }

  auto it = mFileMeshRegistry.find(fullPath);
  if (it != mFileMeshRegistry.end()) {
    log::info("Cached mesh file found: {}", fullPath);
    return it->second;
  }

  log::info("Loading mesh file: {}", fullPath);

  glm::mat3 formatTransform(1);
  Assimp::Importer importer;
  uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
                   aiProcess_FlipUVs | aiProcess_PreTransformVertices;
  const aiScene *scene = importer.ReadFile(file, flags);
  if (scene->mRootNode->mMetaData) {
    throw std::runtime_error("Failed to load mesh file: file contains unsupported metadata, " +
                             fullPath);
  }
  if (!scene) {
    throw std::runtime_error("Failed to load scene: " + std::string(importer.GetErrorString()) +
                             ", " + file);
  }

  std::vector<std::shared_ptr<VulkanMaterial>> mats;
  for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
    std::shared_ptr<VulkanMaterial> mat = mContext->createMaterial();
    auto *m = scene->mMaterials[i];
    PBRMaterialUBO matSpec;

    aiColor3D color{0, 0, 0};
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

  std::vector<std::pair<std::shared_ptr<VulkanMesh>, std::shared_ptr<class VulkanMaterial>>>
      results;
  for (uint32_t i = 0; i < scene->mNumMeshes; i++) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    auto mesh = scene->mMeshes[i];
    if (!mesh->HasFaces())
      continue;

    for (uint32_t v = 0; v < mesh->mNumVertices; v++) {
      glm::vec3 normal = glm::vec3(0);
      glm::vec2 texcoord = glm::vec2(0);
      glm::vec3 position = formatTransform * glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y,
                                                       mesh->mVertices[v].z);
      glm::vec3 tangent = glm::vec3(0);
      glm::vec3 bitangent = glm::vec3(0);
      if (mesh->HasNormals()) {
        normal = formatTransform *
                 glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
      }
      if (mesh->HasTextureCoords(0)) {
        texcoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
      }
      if (mesh->HasTangentsAndBitangents()) {
        tangent = formatTransform *
                  glm::vec3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
        bitangent = formatTransform * glm::vec3(mesh->mBitangents[v].x, mesh->mBitangents[v].y,
                                                mesh->mBitangents[v].z);
      }

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

    std::shared_ptr<VulkanMesh> vulkanMesh = std::make_shared<VulkanMesh>(
        mContext->getPhysicalDevice(), mContext->getDevice(), mContext->getCommandPool(),
        mContext->getGraphicsQueue(), vertices, indices, !mesh->HasNormals());
    results.push_back({vulkanMesh, mats[mesh->mMaterialIndex]});
  }
  mFileMeshRegistry[fullPath] = results;
  return results;
}

std::shared_ptr<VulkanTextureData> VulkanResourcesManager::loadTexture(std::string const &file) {
  std::string fullPath = fs::canonical(file);
  if (!fs::is_regular_file(fullPath)) {
    log::error("Texture file not found: {}", fullPath);
    return {};
  }
  auto it = mFileTextureRegistry.find(fullPath);
  if (it != mFileTextureRegistry.end()) {
    log::info("Cached texture found: {}", file);
    return it->second;
  }

  int width, height, nrChannels;
  unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
  auto texture = std::make_shared<VulkanTextureData>(
      mContext->getPhysicalDevice(), mContext->getDevice(),
      vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)});

  texture->setImage(mContext->getPhysicalDevice(), mContext->getDevice(),
                    mContext->getCommandPool(), mContext->getGraphicsQueue(),
                    [&](void *target, vk::Extent2D const &extent) {
                      memcpy(target, data, extent.width * extent.height * 4);
                    });
  stbi_image_free(data);
  mFileTextureRegistry[fullPath] = texture;
  return texture;
}

std::shared_ptr<VulkanTextureData> VulkanResourcesManager::getPlaceholderTexture() {
  if (!mPlaceholderTexture) {
    mPlaceholderTexture = std::make_shared<VulkanTextureData>(
        mContext->getPhysicalDevice(), mContext->getDevice(), vk::Extent2D{1u, 1u});
    mPlaceholderTexture->setImage(mContext->getPhysicalDevice(), mContext->getDevice(),
                                  mContext->getCommandPool(), mContext->getGraphicsQueue(),
                                  [&](void *target, vk::Extent2D const &extent) {});
  }
  return mPlaceholderTexture;
}

} // namespace svulkan
