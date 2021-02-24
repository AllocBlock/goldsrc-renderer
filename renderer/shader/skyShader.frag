#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform SUniformBufferObject
{
    vec3 EyePosition;
} ubo;

//layout(binding = 2) uniform samplerCube uSkyCubeSampler;
layout(binding = 2) uniform sampler2D uSkyCubeSampler;

void main()
{
	vec3 TexCoord = inFragPosition * 0.5 + 0.5;
	outColor = texture(uSkyCubeSampler, TexCoord.xy);
	//outColor = vec4(1.0, 0.0, 0.0, 1.0);
}