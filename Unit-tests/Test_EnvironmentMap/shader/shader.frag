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
	//vec3 TexCoord;
	//outColor = texture(uSkyCubeSampler, TexCoord);
	outColor = vec4(1.0, 0.0, 0.0, 1.0);
}