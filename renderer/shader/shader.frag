#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec3 inFragPosition;
layout(location = 2) in vec3 inFragNormal;
layout(location = 3) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

#define MAX_TEXTURE_NUM 2048 // if need change, you should change this in renderer config as well
layout(binding = 1) uniform SUniformBufferObject
{
    vec3 uEye;
} ubo;
layout(binding = 2) uniform sampler uTexSampler;
layout(binding = 3) uniform texture2D uTextures[MAX_TEXTURE_NUM];
layout(push_constant) uniform SPushConstant 
{
	uint TexIndex;
} uPushConstant;

void main()
{
    vec3 Light = vec3(1.0, 1.0, 1.0);

	//vec3 L = normalize(Light - inFragPosition); // point light
	vec3 L = normalize(Light); // parallel light
	vec3 N = normalize(inFragNormal);
	vec3 V = normalize(ubo.uEye - inFragPosition);
	vec3 H = normalize(L + V);

	float ambient = 0.3;
	float diffuse = dot(L, N);
	float specular = pow(dot(N, H), 20.0);
	if (diffuse <= 0.0)
	{
		diffuse = 0.0;
		specular = 0.0;
	}

	float fShadow = ambient + diffuse + specular;

	uint TexIndex = uPushConstant.TexIndex;
	if (TexIndex > MAX_TEXTURE_NUM) TexIndex = 0;
	vec3 TexColor = texture(sampler2D(uTextures[uPushConstant.TexIndex], uTexSampler), inFragTexCoord).xyz;
    
    //outColor = vec4((inFragColor * 0.5 + TexColor * 0.5) * fShadow, 1.0);
    outColor = vec4(TexColor * fShadow, 1.0);
}