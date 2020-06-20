#include "sapien_vulkan/camera_controller.h"
#include <algorithm>

namespace svulkan
{

FPSCameraController::FPSCameraController(Camera &camera, glm::vec3 forward, glm::vec3 up)
    : mCamera(&camera),
      mForward(glm::normalize(forward)),
      mUp(glm::normalize(up)),
      mLeft(glm::cross(mUp, mForward)) {
  mInitialRotation = glm::mat3(-mLeft, mUp, -mForward);
}

void FPSCameraController::setRPY(float roll, float pitch, float yaw) {
  mRPY = {roll, pitch, yaw};
  update();
}

void FPSCameraController::setXYZ(float x, float y, float z) {
  mXYZ = {x,y,z};
  update();
}

void FPSCameraController::move(float forward, float left, float up) {
  auto pose = glm::angleAxis(mRPY.z, mUp) * glm::angleAxis(-mRPY.y, mLeft) * glm::angleAxis(mRPY.x, mForward);
  mXYZ += pose * mForward * forward + pose * mLeft * left + pose * mUp * up;
  update();
}

void FPSCameraController::rotate(float roll, float pitch, float yaw) {
  mRPY += glm::vec3{roll, pitch, yaw};
  update();
}

void FPSCameraController::update() {
  mRPY.y = std::clamp(mRPY.y, -1.57f, 1.57f); 
  if (mRPY.z >= 3.15)  {
    mRPY.z -= 2 * glm::pi<float>();
  } else if (mRPY.z <= -3.15) {
    mRPY.z += 2 * glm::pi<float>();
  }

  mCamera->rotation = glm::angleAxis(mRPY.z, mUp) * glm::angleAxis(-mRPY.y, mLeft) *
                      glm::angleAxis(mRPY.x, mForward) * mInitialRotation;
  mCamera->position = mXYZ;
}

}
