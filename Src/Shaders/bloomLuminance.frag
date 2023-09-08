#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(binding = 0) uniform sampler2D uTexInput;

layout(location = 0) out vec4 outColor;

#define THRESHOLD 0.6

float luminance(vec3 color)
{
    return 0.2125 * color.r + 0.7154 * color.g + 0.0721 * color.b; 
}

void main()
{
    vec3 color = texture(uTexInput, inFragTexCoord).rgb;
    float l = clamp(luminance(color) - THRESHOLD, 0.0, 1.0);
    outColor = vec4(color * l, 1.0);
}