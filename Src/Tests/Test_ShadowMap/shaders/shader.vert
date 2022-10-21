#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Project;
    mat4 View;
    mat4 Model;
    mat4 LightVP;
} ubo;

layout(location = 0) out vec3 outFragPosition;
layout(location = 1) out vec3 outFragNormal;
layout(location = 2) out vec4 outFragPositionFromLight;

void main()
{
    gl_Position = ubo.Project * ubo.View * ubo.Model * vec4(inPosition, 1.0);

    outFragPosition = vec3(ubo.Model * vec4(inPosition, 1.0));
    outFragNormal = (ubo.Model * vec4(inNormal, 1.0)).xyz;
    outFragPositionFromLight = (ubo.LightVP * vec4(inPosition, 1.0));
}