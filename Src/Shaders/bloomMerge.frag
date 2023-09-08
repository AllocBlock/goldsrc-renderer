#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    float BloomFactor;
} ubo;
layout(binding = 1) uniform sampler2D uTexBase;
layout(binding = 2) uniform sampler2D uTexBlur;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 color = texture(uTexBase, inFragTexCoord).rgb + texture(uTexBlur, inFragTexCoord).rgb * ubo.BloomFactor;
    outColor = vec4(color, 1.0);
}