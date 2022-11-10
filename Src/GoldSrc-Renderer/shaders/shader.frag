#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec3 inFragPosition;
layout(location = 2) in vec3 inFragNormal;
layout(location = 3) in vec2 inFragTexCoord;
layout(location = 4) in vec2 inFragLightmapCoord;
layout(location = 5) flat in uint inFragTexIndex;

layout(location = 0) out vec4 outColor;

#define MAX_TEXTURE_NUM 2048 // if need change, you should change this in renderer config as well
layout(binding = 1) uniform SUniformBufferObject
{
    vec3 uEye;
} ubo;
layout(binding = 2) uniform sampler uTexSampler;
layout(binding = 3) uniform texture2D uTextures[MAX_TEXTURE_NUM];
layout(binding = 4) uniform texture2D uLightmap;
layout(push_constant) uniform SPushConstant 
{
	bool UseLightmap;
	float Opacity;
} uPushConstant;

#define GAMMA 2.2

float srgbToLinear(float v)
{
	return pow(v, 1/GAMMA);
}

vec3 srgbToLinear(vec3 vColor)
{
	return vec3(srgbToLinear(vColor.r), srgbToLinear(vColor.g), srgbToLinear(vColor.b));
}

float linearToSrgb(float v)
{
	return pow(v, GAMMA);
}

vec3 linearToSrgb(vec3 vColor)
{
	return vec3(linearToSrgb(vColor.r), linearToSrgb(vColor.g), linearToSrgb(vColor.b));
}

void main()
{
	uint TexIndex = inFragTexIndex;
	if (TexIndex > MAX_TEXTURE_NUM) TexIndex = 0;
	vec4 TexColor = texture(sampler2D(uTextures[inFragTexIndex], uTexSampler), inFragTexCoord);
	//TexColor.rgb = srgbToLinear(TexColor.rgb);

    if (uPushConstant.UseLightmap)
	{
		vec3 LightmapColor = texture(sampler2D(uLightmap, uTexSampler), inFragLightmapCoord).xyz;
		outColor = vec4(TexColor.rgb * LightmapColor, TexColor.a * uPushConstant.Opacity);
	}
	else
	{
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
		//outColor = vec4(linearToSrgb(TexColor.rgb * shadow), TexColor.a * uPushConstant.Opacity);
		outColor = vec4(srgbToLinear(TexColor.rgb) * shadow, TexColor.a * uPushConstant.Opacity);
	}
}