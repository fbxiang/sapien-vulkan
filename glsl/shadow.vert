#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0, set = 0) uniform LightUBO {
  mat4 lsMatrix;
} lightUBO;

layout(binding = 0, set = 1) uniform ObjectUBO {
  mat4 modelMatrix;
  uvec4 segmentation;
  mat4 userData;
} objectUBO;


layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;


void main() {
  gl_Position = lightUBO.lsMatrix * objectUBO.modelMatrix * vec4(pos, 1);
}
