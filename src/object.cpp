#include "sapien_vulkan/object.h"

namespace svulkan
{
Object::Object(std::unique_ptr<VulkanObject> vulkanObject) : mVulkanObject(std::move(vulkanObject)) {}

glm::mat4 Object::getModelMat() const {
  glm::mat4 t = glm::toMat4(mTransform.rotation);
  t[3][0] = mTransform.position.x;
  t[3][1] = mTransform.position.y;
  t[3][2] = mTransform.position.z;

  t[0][0] *= mTransform.scale[0];
  t[1][1] *= mTransform.scale[1];
  t[2][2] *= mTransform.scale[2];
  return t;
}

void Object::updateVulkanObject() {
  if (mVulkanObject) {
    copyToDevice<ObjectUBO>(mVulkanObject->mDevice, mVulkanObject->mUBO.mMemory.get(),
                            {mGlobalModelMatrixCache, {mSegmentId, 0, 0, 0}});
  }
}

void Object::addChild(std::unique_ptr<Object> child) { mChildren.push_back(std::move(child)); }

}
