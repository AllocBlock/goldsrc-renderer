#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform SUniformBufferObject
{
    mat4 Model;
    mat4 View;
    mat4 Proj;
} ubo;

void main()
{
    gl_Position = ubo.Proj * ubo.View * ubo.Model * vec4(inPosition, 1.0);
    fragColor = vec3(1.0, 1.0, 0.0);
}