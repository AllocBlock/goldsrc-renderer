#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(binding = 0) uniform sampler2D uTexInput;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texture(uTexInput, inFragTexCoord).rgb, 1.0);
}