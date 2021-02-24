#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outFragPosition;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Proj;
    mat4 View;
    vec3 EyePosition;
} ubo;

void main()
{
    vec3 Offset = ubo.EyePosition;
    gl_Position = ubo.Proj * ubo.View * vec4(inPosition + Offset, 1.0);
    //gl_Position = vec4(inPosition, 1.0);
    //outFragPosition = (ubo.View * vec4(inPosition, 1.0)).xyz;
    outFragPosition = inPosition;
}