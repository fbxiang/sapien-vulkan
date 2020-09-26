#pragma once
#include "sapien_vulkan/common/glm_common.h"
#include "vulkan_buffer.h"
#include <memory>

namespace svulkan {

struct Vertex {
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 uv{};
  glm::vec3 tangent{};
  glm::vec3 bitangent{};

  static std::vector<std::pair<vk::Format, std::uint32_t>> const &getFormatOffset() {
    static_assert(offsetof(Vertex, position) == 0);
    static_assert(offsetof(Vertex, normal) == 12);
    static_assert(offsetof(Vertex, uv) == 24);
    static_assert(offsetof(Vertex, tangent) == 32);
    static_assert(offsetof(Vertex, bitangent) == 44);
    static std::vector<std::pair<vk::Format, std::uint32_t>> v = {
        {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
        {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
        {vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},
        {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent)},
        {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, bitangent)}};
    return v;
  }

  Vertex(glm::vec3 p = {0, 0, 0}, glm::vec3 n = {0, 0, 0}, glm::vec2 u = {0, 0},
         glm::vec3 t = {0, 0, 0}, glm::vec3 b = {0, 0, 0});
};

struct VulkanMesh {
  std::unique_ptr<VulkanBufferData> mVertexBuffer;
  std::unique_ptr<VulkanBufferData> mIndexBuffer;
  uint32_t mVertexCount;
  uint32_t mIndexCount;

  static void recalculateNormals(std::vector<Vertex> &vertices,
                                 const std::vector<uint32_t> &indices);

  VulkanMesh(vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool,
             vk::Queue queue, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
             bool calculateNormals = false);

  VulkanMesh(VulkanMesh const &other) = delete;
  VulkanMesh(VulkanMesh &&other) = default;
  VulkanMesh &operator=(VulkanMesh const &other) = delete;
  VulkanMesh &operator=(VulkanMesh &&other) = default;
  ~VulkanMesh() = default;

  static std::shared_ptr<VulkanMesh> CreateCube(vk::PhysicalDevice physicalDevice,
                                                vk::Device device, vk::CommandPool commandPool,
                                                vk::Queue queue);

  std::vector<Vertex> downloadVertices(vk::PhysicalDevice physicalDevice, vk::Device device,
                                       vk::CommandPool commandPool, vk::Queue queue) const;
  std::vector<uint32_t> downloadIndices(vk::PhysicalDevice physicalDevice, vk::Device device,
                                        vk::CommandPool commandPool, vk::Queue queue) const;
};

} // namespace svulkan
