#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;

layout(binding = 1) uniform SUniformBufferObject
{
    vec3 EyePos;
} ubo;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 BaseColor = vec3(0.0, 1.0, 0.0);

	vec3 E = normalize(ubo.EyePos - inFragPosition);
	vec3 N = normalize(inFragNormal);

	float Intensity = dot(E, N) * 0.35 + 0.65; // map [-1, 1] to [0.3, 1.0]
	
	outColor = vec4(Intensity * BaseColor, 1.0);
	//outColor = vec4(inFragNormal * 0.5 + 0.5, 1.0);
}