#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Project;
    mat4 View;
    mat4 Model;
} ubo;

layout(location = 0) out vec3 outFragPosition;
layout(location = 1) out vec3 outFragNormal;
layout(location = 2) out vec3 outFragColor;

void main()
{
    gl_PointSize = 1;
    gl_Position = ubo.Project * ubo.View * ubo.Model * vec4(inPosition, 1.0);

    outFragPosition = (ubo.Model * vec4(inPosition, 1.0)).xyz;
    outFragNormal = vec3(0, 0, 0);
    outFragColor = inColor;
}