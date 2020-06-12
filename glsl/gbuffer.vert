#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0, set = 1) uniform CameraUBO {
  mat4 viewMatrix;
  mat4 projectionMatrix;
  mat4 viewMatrixInverse;
  mat4 projectionMatrixInverse;
} cameraUBO;

layout(binding = 0, set = 2) uniform ObjectUBO {
  mat4 modelMatrix;
  uvec4 segmentation;
} objectUBO;


layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec4 outPosition;

void main() {
  mat4 modelView = cameraUBO.viewMatrix * objectUBO.modelMatrix;
  mat4 mvp = cameraUBO.projectionMatrix * modelView;
  mat4 normalMat = transpose(inverse(modelView));

  gl_Position = mvp * vec4(pos, 1);
  outPosition = gl_Position;
  outNormal = normalMat * vec4(normal, 0);
}
