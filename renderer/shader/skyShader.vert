#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outFragPosition;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Project;
    mat4 View;
    vec3 EyePosition;
} ubo;

void main()
{
     gl_Position = ubo.Project * ubo.View * vec4(inPosition + ubo.EyePosition, 1.0);
     //gl_Position = vec4(inPosition, 1.0);
     //outFragPosition = (ubo.View * vec4(inPosition, 1.0)).xyz;
     outFragPosition = inPosition;
}