#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform SUniformBufferObject
{
    vec3 EyePosition;
} ubo;
layout(binding = 1) uniform sampler uTexSampler;
layout(binding = 2) uniform texture2D uTextures[6];

void main()
{
	outColor = vec4(inFragPosition, 0.0);
}