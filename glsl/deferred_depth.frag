#version 450 

layout(set = 2, binding = 4) uniform sampler2D depthSampler;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
  float depth = texture(depthSampler, inUV).x;
  outColor = vec4(depth, depth, depth, 1.f);
}
