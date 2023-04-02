#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outFragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 Proj;
    mat4 View;
    vec3 EyePosition;
    vec3 EyeDirection;
} ubo;

layout(push_constant) uniform SPushConstant 
{
    uint TexIndex;
    uint BlendType;
    vec3 Position;
    vec3 Scale;
} uPushConstant;

#define _BLEND_TYPE_NORMAL 0x00
#define _BLEND_TYPE_MASK 0x01
#define _BLEND_TYPE_ALPHA 0x02

void main()
{
    vec3 X = normalize(ubo.EyePosition - uPushConstant.Position);
    vec3 Y = normalize(cross(vec3(0.0, 0.0, 1.0), X));
    vec3 Z = normalize(cross(X, Y));
    mat3 Rotate = mat3(X, Y, Z);
    vec3 World = (Rotate * inPosition * uPushConstant.Scale) + uPushConstant.Position;

    gl_Position = ubo.Proj * ubo.View * vec4(World, 1.0);
    outFragTexCoord = inTexCoord;
}