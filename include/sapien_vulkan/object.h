#pragma once
#include "common/glm_common.h"
#include <memory>
#include "internal/vulkan_object.h"
#include <vector>

namespace svulkan 
{

class Object
{
  std::unique_ptr<VulkanObject> mVulkanObject = nullptr;
  Object *mParent = nullptr;
  std::vector<std::unique_ptr<Object>> mChildren = {};
  uint32_t mObjectId = 0u;  // unique id for this object
  uint32_t mSegmentId = 0u;  // custom id

  class Scene *mScene = nullptr;

  bool mMarkedForRemove = false;

 public:
  // cache
  glm::mat4 mGlobalModelMatrixCache;

 public:
  std::string mName = "";
  float mVisibility = 1.f;  // this visibility should only be used in editor rendering
  Transform mTransform = {};

  Object(std::unique_ptr<VulkanObject> vulkanObject);

  Object(Object const &other) = delete;
  Object &operator=(Object const &other) = delete;

  Object(Object &&other) = default;
  Object &operator=(Object &&other) = default;

  ~Object() = default;

  glm::mat4 getModelMat() const;

  inline void setScene(Scene *scene) { mScene = scene; }
  inline Scene *getScene() const { return mScene; }

  void addChild(std::unique_ptr<Object> child);

  void updateVulkanObject();

  inline VulkanObject *getVulkanObject() { return mVulkanObject.get(); }

  inline std::vector<std::unique_ptr<Object>> const &getChildren() { return mChildren; }

  inline void markForRemove() { mMarkedForRemove = true; }
  inline bool isMarkedForRemove() const { return mMarkedForRemove; }

  inline void setObjectId(uint32_t id) { mObjectId = id; }
  inline uint32_t getObjectId() { return mObjectId; }

  inline void setSegmentId(uint32_t id) { mSegmentId = id; }
  inline uint32_t getSegmentId() { return mSegmentId; }

  void updateMaterial(PBRMaterialUBO material);
  inline std::shared_ptr<VulkanMaterial> getMaterial() { return mVulkanObject->mMaterial; }
};

}
