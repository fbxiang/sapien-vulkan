#pragma once
#include "glm_common.h"
#include "vulkan_buffer.h"
#include <memory>

namespace svulkan {

struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 uv{};
  glm::vec3 tangent{};
  glm::vec3 bitangent{};
};

struct VulkanMesh {
  std::unique_ptr<VulkanBufferData> mVertexBuffer;
  std::unique_ptr<VulkanBufferData> mIndexBuffer;
  uint32_t mVertexCount;
  uint32_t mIndexCount;

  static void recalculateNormals(std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

  VulkanMesh(vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool,
             vk::Queue queue, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
             bool calculateNormals = false);

  VulkanMesh(VulkanMesh const &other) = delete;
  VulkanMesh(VulkanMesh &&other) = default;
  VulkanMesh &operator=(VulkanMesh const &other) = delete;
  VulkanMesh &operator=(VulkanMesh &&other) = default;
  ~VulkanMesh() = default;
};
}
