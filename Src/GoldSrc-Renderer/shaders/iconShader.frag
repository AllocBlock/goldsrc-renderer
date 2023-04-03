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
    float Scale;
    vec3 Position;
} uPushConstant;

#define _BLEND_TYPE_NORMAL 0x00
#define _BLEND_TYPE_INDEXED_TRANSPARENT 0x01

void main()
{
	outColor = texture(sampler2D(uTextures[uPushConstant.TexIndex], uTexSampler), inFragTexCoord);
    if (uPushConstant.BlendType == _BLEND_TYPE_NORMAL)
    {
        outColor.a = 1.0;
    }
    else if (uPushConstant.BlendType == _BLEND_TYPE_INDEXED_TRANSPARENT)
    {
        if (outColor.a < 0.5)
            discard;
    }
}