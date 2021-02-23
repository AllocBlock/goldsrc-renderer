#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inFragPosition;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(inFragPosition, 0.0);
}