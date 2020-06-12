#version 450 

// camera global ubo
layout(set = 0, binding = 0) uniform GlobalUBO {
  mat4 viewMatrix;
  mat4 projectionMatrix;
  mat4 viewMatrixInverse;
  mat4 projectionMatrixInverse;
} globalUBO;


// scene ubo, lighting
#define MAX_DIRECTIONAL_LIGHTS 3
#define MAX_POINT_LIGHTS 10
struct PointLight {
  vec4 position;
  vec4 emission;
};
struct DirectionalLight {
  vec4 direction;
  vec4 emission;
};
layout(set = 1, binding = 0) uniform SceneUBO {
  DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
  PointLight pointLights[MAX_POINT_LIGHTS];
} sceneUBO;

layout(set = 2, binding = 0) uniform sampler2D albedoSampler;
layout(set = 2, binding = 1) uniform sampler2D specularSampler;
layout(set = 2, binding = 2) uniform sampler2D normalSampler;
layout(set = 2, binding = 3) uniform sampler2D depthSampler;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

vec4 tex2camera(vec4 pos) {
  vec4 ndc = vec4(pos.xy * 2.f - 1.f, pos.z, 1.f);
  vec4 cam = globalUBO.projectionMatrixInverse * ndc;
  return cam / cam.w;
}

vec4 world2camera(vec4 pos) {
  return globalUBO.viewMatrix * pos;
}

vec4 getCameraSpacePosition(vec2 texcoord) {
  float depth = texture(depthSampler, texcoord).x;
  return tex2camera(vec4(texcoord, depth, 1.f));
}

vec3 getBackgroundColor(vec3 texcoord) {
  float r = sqrt(texcoord.x * texcoord.x + texcoord.z * texcoord.z);
  float angle = atan(texcoord.y / r) * 57.3;

  vec3 horizonColor = vec3(0.9, 0.9, 0.9);
  vec3 zenithColor = vec3(0.522, 0.757, 0.914);
  vec3 groundColor = vec3(0.5, 0.410, 0.271);
  
  return mix(mix(zenithColor, horizonColor, smoothstep(15.f, 5.f, angle)),
             groundColor,
             smoothstep(-5.f, -15.f, angle));
}

// lambertian model
float lambertian(vec3 l, vec3 v, vec3 n) {
  return clamp(dot(n, l), 0.f, 1.f);
}

// The Oren-Nayar shading model
float orenNayar(vec3 l, vec3 v, vec3 n, float r) {
  float a = r * r;
  float NoL = clamp(dot(n, l), 0, 1);
  float NoV = clamp(dot(n, v), 0, 1);
  float LoV = clamp(dot(l, v), 0, 1);
  float NLNV = NoL * NoV;

  if (NoL < 0 || NoV <0) return 0;

  float A = 1 - 0.5 * (a / (a + 0.33));
  float B = 0.45 * (a / (a + 0.09));
  float C = max(0, LoV - NLNV) / max(NoL, NoV);

  return min(max(0, NoL) * (A + B * C), 1);
}

float ggx (vec3 L, vec3 V, vec3 N, float roughness, float F0) {
  float alpha = roughness*roughness;
  vec3 H = normalize(L + V);
  float dotLH = clamp(dot(L,H), 0, 1);
  float dotNH = clamp(dot(N,H), 0, 1);
  float dotNL = clamp(dot(N,L), 0, 1);

  float alphaSqr = alpha * alpha;
  float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
  float D = alphaSqr / (3.141592653589793 * denom * denom);
  float F = F0 + (1.0 - F0) * pow(1.0 - dotLH, 5.0);
  float k = 0.5 * alpha;
  float k2 = k * k;
  return dotNL * D * F / (dotLH*dotLH*(1.0-k2)+k2);
}

void main() {
  vec3 albedo = texture(albedoSampler, inUV).xyz;
  vec4 specular = texture(specularSampler, inUV);
  float roughness = 2 / (2 + specular.a);
  vec3 normal = texture(normalSampler, inUV).xyz * 2 - 1;
  vec4 csPosition = getCameraSpacePosition(inUV);

  vec3 camDir = -normalize(csPosition.xyz);
  vec3 color = vec3(0.f);


  for (int i = 0; i < 1; ++i) {
    vec3 lightDir = -normalize((globalUBO.viewMatrix *
                                vec4(sceneUBO.directionalLights[i].direction.xyz, 0)).xyz);
    color += albedo * sceneUBO.directionalLights[i].emission.xyz *
             orenNayar(lightDir, camDir, normal, 0.3f) * step(0, dot(lightDir, normal));
    color += specular.rgb * sceneUBO.directionalLights[i].emission.xyz *
             ggx(lightDir, camDir, normal, roughness, 0.05) * step(0, dot(lightDir, normal));
  }

  float depth = texture(depthSampler, inUV).x;
  if (depth == 1) {
    outColor = vec4(getBackgroundColor((globalUBO.viewMatrixInverse * csPosition).xyz), 1.f);
  } else {
    outColor = vec4(color, 1);
  }
  // outColor = vec4(depth, depth, depth, 1);
  // outColor = vec4(normal, 1);
  // outColor = vec4(csPosition.xy, 0, 1);
  // outColor = vec4(wsPosition.xyz, 1);
  // outColor = vec4(wsNormal, 1);
  // outColor = vec4(ffnormal, 1);
  // outColor = vec4(-csPosition.xyz / 6, 1);
}
