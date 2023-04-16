#version 450

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
	vec3 Color;
} uConstant;

layout(location = 0) out vec3 outFragPosition;
layout(location = 1) out vec3 outFragNormal;
layout(location = 2) out vec3 outFragColor;

void main()
{
    gl_Position = ubo.Project * ubo.View * uConstant.Model * vec4(inPosition, 1.0);

    outFragPosition = (uConstant.Model * vec4(inPosition, 1.0)).xyz;
    outFragNormal = (transpose(inverse(uConstant.Model)) * vec4(inNormal, 1.0)).xyz;
    outFragColor = uConstant.Color;
}