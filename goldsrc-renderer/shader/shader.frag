#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_TEXTURE_NUM 2048

layout(location = 0) in vec3 fragColor;
layout(location = 1) in float texIndexf;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragShadow;

layout(location = 0) out vec4 outColor;


layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[MAX_TEXTURE_NUM];

void main() {
	int texIndex = int(texIndexf);
	if (texIndexf - float(texIndex) > 0.95) texIndex++;		// 解决精度丢失问题
	outColor = texture(sampler2D(textures[texIndex], texSampler), fragTexCoord);
	outColor = vec4(fragShadow, 1.0) * outColor;
}