#include "vulkan_mesh.h"


namespace svulkan {

void VulkanMesh::recalculateNormals(std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
  for (auto &v : vertices) {
    v.normal = {0, 0, 0};
    v.tangent = {0, 0, 0};
    v.bitangent = {0, 0, 0};
  }
  for (size_t i = 0; i < indices.size(); i += 3) {
    uint32_t i1 = indices[i];
    uint32_t i2 = indices[i + 1];
    uint32_t i3 = indices[i + 2];
    Vertex &v1 = vertices[i1];
    Vertex &v2 = vertices[i2];
    Vertex &v3 = vertices[i3];

    glm::vec3 normal = glm::normalize(glm::cross(v2.position - v1.position, v3.position - v1.position));
    if (std::isnan(normal.x)) {
      continue;
    }
    v1.normal += normal;
    v2.normal += normal;
    v3.normal += normal;
  }

  for (size_t i = 0; i < vertices.size(); i++) {
    if (vertices[i].normal == glm::vec3(0))
      continue;
    vertices[i].normal = glm::normalize(vertices[i].normal);
  }
}


VulkanMesh::VulkanMesh(vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool,
           vk::Queue queue, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices,
                       bool calculateNormals) : mVertexCount(vertices.size()), mIndexCount(indices.size())
{
  mVertexBuffer = std::make_unique<VulkanBufferData>(physicalDevice, device, sizeof(Vertex) * vertices.size(),
                  vk::BufferUsageFlagBits::eVertexBuffer |
                  vk::BufferUsageFlagBits::eTransferDst,
                  vk::MemoryPropertyFlagBits::eDeviceLocal);

  mIndexBuffer = std::make_unique<VulkanBufferData>(physicalDevice, device, sizeof(uint32_t) * indices.size(),
                                                     vk::BufferUsageFlagBits::eIndexBuffer |
                                                     vk::BufferUsageFlagBits::eTransferDst,
                                                     vk::MemoryPropertyFlagBits::eDeviceLocal);
  if (calculateNormals) {
    recalculateNormals(vertices, indices);
  }
  mVertexBuffer->uploadNoWait(physicalDevice, device, commandPool, queue, vertices, sizeof(Vertex));
  mIndexBuffer->uploadNoWait(physicalDevice, device, commandPool, queue, indices, sizeof(uint32_t));
  queue.waitIdle();
}

}
