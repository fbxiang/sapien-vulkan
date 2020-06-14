#include "scene.h"

namespace svulkan
{

void Scene::updateUBO() {
  if (mLightUpdated) {
    SceneUBO ubo {};
    ubo.ambientLight = ambientLight;
    for (uint32_t i = 0; i < std::min<uint32_t>(NumDirectionalLights, directionalLights.size()); ++i) {
      ubo.directionalLights[i] = directionalLights[i];
    }
    for (uint32_t i = 0; i < std::min<uint32_t>(NumPointLights, pointLights.size()); ++i) {
      ubo.pointLights[i] = pointLights[i];
    }
    mVulkanScene->updateUBO(ubo);
    mLightUpdated = false;
  }
}

Scene::Scene(std::unique_ptr<VulkanScene> vulkanScene) : mVulkanScene(std::move(vulkanScene)) {}

void Scene::addObject(std::unique_ptr<Object> obj) {
  obj->setScene(this);
  objects.push_back(std::move(obj));
}


void Scene::forceRemove() {
  objects.erase(std::remove_if(objects.begin(), objects.end(),
                               [](std::unique_ptr<Object> &o) { return o->isMarkedForRemove(); }),
                objects.end());
}


void Scene::removeObject(Object *obj) {
  auto s = obj->getScene();
  if (s != this) {
    return;
  }
  obj->markForRemove();
}

void Scene::removeObjectsByName(std::string name) {
  for (auto &o : objects) {
    if (o->mName == name) {
      o->markForRemove();
    }
  }
}

void Scene::setAmbientLight(glm::vec4 const &light) {
  ambientLight = light;
  mLightUpdated = true;
}

void Scene::addPointLight(PointLight const &light) {
  pointLights.push_back(light);
  mLightUpdated = true;
}

void Scene::addDirectionalLight(DirectionalLight const &light) {
  directionalLights.push_back(light);
  mLightUpdated = true;
}

void Scene::addParalleloGramLight(ParallelogramLight const &light) {
  parallelogramLights.push_back(light);
  mLightUpdated = true;
}

static void prepareObjectTree(Object *obj, const glm::mat4 &parentModelMat,
                              std::vector<Object *> &opaque, std::vector<Object *> &transparent) {
  obj->mGlobalModelMatrixCache = parentModelMat * obj->getModelMat();
  if (obj->getVulkanObject() && obj->mVisibility > 0.f) {
    if (obj->mVisibility < 1.f) {
      transparent.push_back(obj);
    } else {
      opaque.push_back(obj);
    }
  }
  for (auto &c : obj->getChildren()) {
    prepareObjectTree(c.get(), obj->mGlobalModelMatrixCache, opaque, transparent);
  }
}

void Scene::prepareObjectsForRender() {
  forceRemove();
  opaque_objects.clear();
  transparent_objects.clear();
  for (auto &obj : objects) {
    prepareObjectTree(obj.get(), glm::mat4(1.f), opaque_objects, transparent_objects);
  }
}

}
