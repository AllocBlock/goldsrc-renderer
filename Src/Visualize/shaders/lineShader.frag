#version 450

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec3 inFragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform SUniformBufferObject
{
    vec3 uEye;
} ubo;

void main()
{
	outColor = vec4(inFragColor, 1.0);
}