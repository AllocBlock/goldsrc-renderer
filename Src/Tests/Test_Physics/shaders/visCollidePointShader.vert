#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform SUniformBufferObject
{
    mat4 Project;
    mat4 View;
} ubo;

layout(push_constant) uniform SPushConstant
{
	mat4 Model;
} uConstant;

layout(location = 0) out vec3 outFragPosition;

void main()
{
    gl_Position = ubo.Project * ubo.View * uConstant.Model * vec4(inPosition, 1.0);

    outFragPosition = (uConstant.Model * vec4(inPosition, 1.0)).xyz;
}