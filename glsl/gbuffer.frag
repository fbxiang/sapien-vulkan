#version 450

layout(binding = 0, set = 3) uniform MaterialUBO {
  vec4 baseColor;  // rgba
  float specular;
  float roughness;
  float metallic;
  float transparency;
} materialUBO;


// struct PointLight {
//   vec4 position;
//   vec4 emission;
// };

// struct DirectionalLight {
//   vec4 direction;
//   vec4 emission;
// };

// #define MAX_DIRECTIONAL_LIGHTS 10
// #define MAX_POINT_LIGHTS 100
// layout(binding = 0, set = 3) uniform SceneUBO {
//   DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
//   PointLight pointLights[MAX_POINT_LIGHTS];
// } sceneUBO;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in mat3 inTbn;
layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outSpecular;
layout(location = 3) out vec4 outNormal;

void main() {
  outPosition = inPosition;
  outAlbedo = vec4(1,0,1,1);
  outSpecular = vec4(0,0,0,0);
  outNormal = vec4(normalize(inTbn * vec3(0,0,1)), 0) * 0.5 + 0.5;
}
