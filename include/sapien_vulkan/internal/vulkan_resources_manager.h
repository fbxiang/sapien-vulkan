#pragma once

#include "sapien_vulkan/internal/vulkan.h"
#include <map>

namespace svulkan {
class VulkanContext;

class VulkanResourcesManager {
  VulkanContext *mContext;

  std::shared_ptr<VulkanMesh> mCubeMesh{};
  std::shared_ptr<VulkanMesh> mSphereMesh{};
  std::shared_ptr<VulkanMesh> mYZPlaneMesh{};
  std::map<
      std::string,
      std::vector<std::pair<std::shared_ptr<VulkanMesh>, std::shared_ptr<class VulkanMaterial>>>>
      mFileMeshRegistry{};

  std::map<std::string, std::shared_ptr<VulkanTextureData>> mFileTextureRegistry{};
  std::shared_ptr<VulkanTextureData> mPlaceholderTexture{nullptr};

public:
  VulkanResourcesManager(VulkanContext &context);

  std::shared_ptr<VulkanMesh> loadSphere();
  std::shared_ptr<VulkanMesh> loadCube();
  std::shared_ptr<VulkanMesh> loadCapsule(float radius, float halfLength);
  std::shared_ptr<VulkanMesh> loadYZPlane();

  std::vector<std::pair<std::shared_ptr<VulkanMesh>, std::shared_ptr<class VulkanMaterial>>>
  loadFile(std::string const &file);

  std::shared_ptr<VulkanTextureData> loadTexture(std::string const &filename);
  std::shared_ptr<VulkanTextureData> getPlaceholderTexture();
};

}; // namespace svulkan
