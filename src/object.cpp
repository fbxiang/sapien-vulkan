#include "sapien_vulkan/object.h"

namespace svulkan {
Object::Object(std::unique_ptr<VulkanObject> vulkanObject)
    : mVulkanObject(std::move(vulkanObject)) {}

glm::mat4 Object::getModelMat() const {
  glm::mat4 t = glm::toMat4(mTransform.rotation);
  t[0] *= mTransform.scale.x;
  t[1] *= mTransform.scale.y;
  t[2] *= mTransform.scale.z;
  t[3][0] = mTransform.position.x;
  t[3][1] = mTransform.position.y;
  t[3][2] = mTransform.position.z;
  return t;
}

void Object::updateVulkanObject() {
  if (mVulkanObject) {
    copyToDevice<ObjectUBO>(mVulkanObject->mDevice, mVulkanObject->mUBO.mMemory.get(),
                            {mGlobalModelMatrixCache, {mObjectId, mSegmentId, 0, 0}, mUserData});
  }
}

void Object::addChild(std::unique_ptr<Object> child) { mChildren.push_back(std::move(child)); }

void Object::updateMaterial(PBRMaterialUBO material) {
  mVulkanObject->mMaterial->setProperties(material);
}

void Object::setUserData(std::vector<float> userData) {
  if (userData.size() != 16) {
    throw std::invalid_argument("user data must have length 16");
  }
  mUserData = glm::make_mat4(userData.data());
}

std::vector<float> Object::getUserData(std::vector<float> userData) const {
  return {glm::value_ptr(mUserData), glm::value_ptr(mUserData) + 16};
}

} // namespace svulkan
