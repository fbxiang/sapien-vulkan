#version 450 

layout(set = 2, binding = 3) uniform sampler2D normalSampler;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
  vec3 normal = texture(normalSampler, inUV).xyz;
  outColor = vec4(abs(normal), 1.f);
}
