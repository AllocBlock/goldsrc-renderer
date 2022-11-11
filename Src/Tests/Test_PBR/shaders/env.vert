#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 outFragPosition;

void main()
{
    vec4 PosH = vec4(inPosition, 0.0, 1.0);
    outFragPosition = inPosition;
    gl_Position = PosH;
}