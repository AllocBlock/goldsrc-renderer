#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform Ubo {
    mat4 uTransform;
} ubo;

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = ubo.uTransform * vec4(inPosition, 0.0, 1.0);
    fragColor = vec3(1.0, 1.0, 0.0);
}