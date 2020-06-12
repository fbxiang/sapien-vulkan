#pragma once
#include "camera.h"

namespace svulkan
{

class FPSCameraController {
  Camera *mCamera;
  glm::vec3 mRPY{0,0,0};
  glm::vec3 mXYZ{0,0,0};

  glm::vec3 mForward;
  glm::vec3 mUp;
  glm::vec3 mLeft;

  void update();
 public:
  FPSCameraController(Camera &camera, glm::vec3 forward, glm::vec3 up);

  void setRPY(float roll, float pitch, float yaw);
  void setXYZ(float x, float y, float z);

  void move(float forward, float left, float up);
  void rotate(float roll, float pitch, float yaw);
};

}
