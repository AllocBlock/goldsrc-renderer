#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

// TODO: how to dynamic this? by dynamic shader compile?
#define MAX_ICON_NUM 128 // if need change, you should change this in renderer config as well
layout(binding = 1) uniform sampler uTexSampler;
layout(binding = 2) uniform texture2D uTextures[MAX_ICON_NUM];

layout(push_constant) uniform SPushConstant 
{
    uint TexIndex;
    uint BlendType;
    vec3 Position;
    vec3 Scale;
} uPushConstant;

void main()
{
	outColor = texture(sampler2D(uTextures[uPushConstant.TexIndex], uTexSampler), inFragTexCoord);
    // todo: blend type
}