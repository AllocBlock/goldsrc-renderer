#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D uFontTexture;

layout(push_constant) uniform SPushConstant
{
    vec3 Scale;
    vec3 Position;
} uPushConstant;

void main()
{
	float sdf = texture(uFontTexture, inFragTexCoord).r;
    if (sdf < 0.5) discard;
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}