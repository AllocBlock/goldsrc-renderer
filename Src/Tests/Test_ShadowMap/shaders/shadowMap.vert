#version 450

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 MVP;
} ubo;

layout(location = 0) out vec4 outFragPosition;

void main()
{
    vec4 Pos = ubo.MVP * vec4(inPosition, 1.0);
    gl_Position = Pos;

    outFragPosition = Pos;
}