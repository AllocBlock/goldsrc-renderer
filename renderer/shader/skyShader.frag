#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform SUniformBufferObject
{
    mat4 UpCorrection; 
} ubo;

layout(binding = 2) uniform samplerCube uSkyCubeSampler;

void main()
{
	vec3 TexCoord = (ubo.UpCorrection * vec4(inFragPosition, 1.0)).xyz;
	outColor = texture(uSkyCubeSampler, TexCoord);
}