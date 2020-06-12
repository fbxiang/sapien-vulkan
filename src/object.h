#pragma once
#include "common/glm_common.h"
#include <memory>
#include "vulkan_object.h"

namespace svulkan 
{

class Object
{
  std::unique_ptr<VulkanObject> mVulkanObject = nullptr;
  Object *mParent = nullptr;
  std::vector<std::unique_ptr<Object>> mChildren = {};
  uint32_t mSegmentId = 0u;

  class Scene *mScene = nullptr;

  bool mMarkedForRemove = false;

 public:
  // cache
  glm::mat4 mGlobalModelMatrixCache;

 public:
  std::string mName = "";
  float mVisibility = 1.f;
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

  inline void setSegmentId(uint32_t id) { mSegmentId = id; }
  inline uint32_t getSegmentId() { return mSegmentId; }

  void updateVulkanObject();

  inline VulkanObject *getVulkanObject() { return mVulkanObject.get(); }

  inline std::vector<std::unique_ptr<Object>> const &getChildren() { return mChildren; }

  inline void markForRemove() { mMarkedForRemove = true; }
  inline bool isMarkedForRemove() const { return mMarkedForRemove; }
};

}
