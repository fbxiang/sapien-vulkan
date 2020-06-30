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

  glm::quat mInitialRotation;

  void update();
 public:
  FPSCameraController(Camera &camera, glm::vec3 const &forward, glm::vec3 const &up);

  void setRPY(float roll, float pitch, float yaw);
  void setXYZ(float x, float y, float z);

  inline glm::vec3 getRPY() const { return mRPY; };
  inline glm::vec3 getXYZ() const { return mXYZ; };

  void move(float forward, float left, float up);
  void rotate(float roll, float pitch, float yaw);
};


class ArcRotateCameraController {
  Camera *mCamera;

  glm::vec3 mForward;
  glm::vec3 mUp;
  glm::vec3 mLeft;
  glm::quat mInitialRotation;

  glm::vec3 center {};

  float mYaw {0.f};
  float mPitch {0.f};
  float mR {1.f};

  void update();
 public:
  ArcRotateCameraController(Camera &camera, glm::vec3 const &forward, glm::vec3 const &up);
  void setCenter(float x, float y, float z);
  void rotateYawPitch(float d_yaw, float d_pitch);
  void setYawPitch(float yaw, float pitch);
  void zoom(float in);
  void setRadius(float r);

  inline glm::vec2 getYawPitch() const { return {mYaw, mPitch}; };
  inline glm::vec3 getCenter() const { return center; };
  inline float getRadius() const { return mR; };

};

}
