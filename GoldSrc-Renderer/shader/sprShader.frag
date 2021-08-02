#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

#define MAX_SPRITE_NUM 2048 // if need change, you should change this in renderer config as well
layout(binding = 1) uniform sampler uTexSampler;
layout(binding = 2) uniform texture2D uTextures[MAX_SPRITE_NUM];

layout(push_constant) uniform SPushConstant 
{
	vec3 Origin;
	uint TexIndex;
} uPushConstant;

void main()
{
	outColor = texture(sampler2D(uTextures[uPushConstant.TexIndex], uTexSampler), inFragTexCoord);
	outColor = vec4(1.0, 0.0, 0.0, 1.0);
}