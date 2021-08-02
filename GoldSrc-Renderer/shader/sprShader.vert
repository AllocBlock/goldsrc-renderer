#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outFragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Proj;
    mat4 View;
    vec3 EyePosition;
} ubo;

layout(push_constant) uniform SPushConstant 
{
	vec3 Origin;
	uint TexIndex;
} uPushConstant;

void main()
{
    vec3 Position = inPosition + uPushConstant.Origin;
    gl_Position = ubo.Proj * ubo.View * vec4(Position, 1.0);
    gl_Position = vec4(Position, 1.0);
    outFragTexCoord = inTexCoord;
}