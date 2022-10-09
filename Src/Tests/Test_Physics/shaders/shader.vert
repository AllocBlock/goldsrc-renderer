#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Project;
    mat4 View;
} ubo;

layout(push_constant) uniform SPushConstant 
{
	mat4 Model;
	mat4 NormalModel;
} uConstant;

layout(location = 0) out vec3 outFragPosition;
layout(location = 1) out vec3 outFragNormal;

void main()
{
    gl_Position = ubo.Project * ubo.View * uConstant.Model * vec4(inPosition, 1.0);

    outFragPosition = (uConstant.Model * vec4(inPosition, 1.0)).xyz;
    outFragNormal = mat3(uConstant.NormalModel) * inNormal;
}