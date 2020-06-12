#pragma once
#include "light.h"
#include "uniform_buffers.h"
#include "object.h"
#include "vulkan_scene.h"

namespace svulkan
{
class Scene {

  std::vector<std::unique_ptr<Object>> objects {};
  std::vector<Object *> opaque_objects {};
  std::vector<Object *> transparent_objects {};

  std::vector<PointLight> pointLights {};
  std::vector<DirectionalLight> directionalLights {};
  std::vector<ParallelogramLight> parallelogramLights {};
  glm::vec3 ambientLight {0, 0, 0};

  std::unique_ptr<VulkanScene> mVulkanScene {};

  void updateVulkanScene();

 public:
  Scene(std::unique_ptr<VulkanScene> vulkanScene);

  inline VulkanScene *getVulkanScene() const { return mVulkanScene.get(); }

 public:
  inline const std::vector<std::unique_ptr<Object>> &getObjects() const { return objects; }
  inline const std::vector<Object *> &getOpaqueObjects() const { return opaque_objects; }
  inline const std::vector<Object *> &getTransparentObjects() const { return transparent_objects; }

  void addObject(std::unique_ptr<Object> obj);
  /*  mark an object for removal */
  void removeObject(Object *obj);
  /*  mark objects for removal */
  void removeObjectsByName(std::string name);
  /* remove objects that are marked to be removed now */
  void forceRemove();

  /* should be called before rendering to update cache */
  void prepareObjectsForRender();

  void setAmbientLight(glm::vec3 const &light);
  void addPointLight(PointLight const &light);
  void addDirectionalLight(DirectionalLight const &light);
  void addParalleloGramLight(ParallelogramLight const &light);

  inline glm::vec3 const &getAmbientLight() const { return ambientLight; }
  inline std::vector<PointLight> const &getPointLights() const { return pointLights; }
  inline std::vector<DirectionalLight> const &getDirectionalLights() const { return directionalLights; }
  inline std::vector<ParallelogramLight> const &getParallelogramLights() const {return parallelogramLights;}
  // TODO: env map
};

}
