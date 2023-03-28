#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

#define MAX_SPRITE_NUM 16 // if need change, you should change this in renderer config as well
layout(binding = 1) uniform sampler uTexSampler;
layout(binding = 2) uniform texture2D uTextures[MAX_SPRITE_NUM];

layout(push_constant) uniform SPushConstant 
{
	uint TexIndex;
    uint SpriteType;
    float Scale;
	vec3 Origin;
	vec3 Angle;
} uPushConstant;

void main()
{
	outColor = texture(sampler2D(uTextures[uPushConstant.TexIndex], uTexSampler), inFragTexCoord);
}