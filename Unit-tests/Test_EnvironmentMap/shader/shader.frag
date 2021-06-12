#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform SUniformBufferObject
{
    vec3 uEye;
} ubo;
layout(binding = 2) uniform samplerCube uSkyCubeSampler;

void main()
{
	vec3 N = normalize(inFragNormal);
	vec3 Eye = normalize(ubo.uEye - inFragPosition);

	float P = dot(N, Eye);
	vec3 Reflect = 2 * P * N - Eye;
	vec3 TexCoord = Reflect;
	outColor = texture(uSkyCubeSampler, TexCoord);
}