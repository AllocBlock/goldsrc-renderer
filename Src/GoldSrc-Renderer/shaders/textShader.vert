#version 450

layout(location = 0) in vec2 inPosition;
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
    vec3 Scale;
    vec3 Position;
} uPushConstant;

void main()
{
    vec3 Z = normalize(-ubo.EyeDirection);
    vec3 X = normalize(cross(vec3(0.0, 1.0, 0.0), Z));
    vec3 Y = normalize(cross(Z, X));
    mat3 Rotate = mat3(X, Y, Z);
    vec3 World = (Rotate * vec3(inPosition, 0.0) * uPushConstant.Scale) + uPushConstant.Position;

    gl_Position = ubo.Proj * ubo.View * vec4(World, 1.0);
    outFragTexCoord = inTexCoord;
}