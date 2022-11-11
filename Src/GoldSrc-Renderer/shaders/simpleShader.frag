#version 450

layout(location = 0) in vec3 inFragPosition;
layout(location = 1) in vec3 inFragNormal;
layout(location = 2) in vec2 inFragTexCoord;
layout(location = 3) flat in uint inFragTexIndex;

layout(location = 0) out vec4 outColor;

#define MAX_TEXTURE_NUM 2048 // if need change, you should change this in renderer config as well
layout(binding = 1) uniform SUniformBufferObject
{
    vec3 uEye;
} ubo;

layout(binding = 2) uniform sampler uTexSampler;
layout(binding = 3) uniform texture2D uTextures[MAX_TEXTURE_NUM];

void main()
{
	uint TexIndex = inFragTexIndex;
	if (TexIndex > MAX_TEXTURE_NUM) TexIndex = 0;
	vec4 TexColor = texture(sampler2D(uTextures[inFragTexIndex], uTexSampler), inFragTexCoord);
    vec3 Light = vec3(1.0, 1.0, 1.0);

	//vec3 L = normalize(Light - inFragPosition); // point light
	vec3 L = normalize(Light); // parallel light
	vec3 N = normalize(inFragNormal);
	vec3 V = normalize(ubo.uEye - inFragPosition);
	vec3 H = normalize(L + V);

	float ambient = 1.0;
	float diffuse = dot(L, N);
	float specular = pow(dot(N, H), 40.0);
	if (diffuse <= 0.0)
	{
		diffuse = 0.0;
		specular = 0.0;
	}

	float shadow = ambient * 0.5 + diffuse * 0.3 + specular * 0.2;
	outColor = vec4(TexColor.rgb * shadow, TexColor.a);
}