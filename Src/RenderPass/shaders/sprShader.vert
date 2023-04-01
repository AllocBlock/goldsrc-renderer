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
    uint SpriteType;
    float Scale;
	vec3 Origin;
	vec3 Angle;
} uPushConstant;

#define _SPRITE_TYPE_PARALLEL_UP_RIGHT 0x00
#define _SPRITE_TYPE_FACING_UP_RIGHT 0x01
#define _SPRITE_TYPE_PARALLEL 0x02
#define _SPRITE_TYPE_ORIENTED 0x03
#define _SPRITE_TYPE_PARALLEL_ORIENTED 0x04

#define _PI 3.14159265358

vec2 rotate(vec2 vP, float vAngle)
{
    float Radian = vAngle / 180 * _PI;
    float Sin = sin(Radian);
    float Cos = cos(Radian);
    vec2 Result;
    Result.x = vP.x * Cos - vP.y * Sin;
    Result.y = vP.x * Sin + vP.y * Cos;
    return Result;
}

void main()
{
   
    vec3 Position;
    switch(uPushConstant.SpriteType)
    {
        case _SPRITE_TYPE_PARALLEL_UP_RIGHT:
        {
            vec3 X = normalize(-ubo.EyeDirection);
            vec3 Y = normalize(cross(vec3(0.0, 0.0, 1.0), X));
            vec3 Z = normalize(vec3(0.0, 0.0, 1.0));
            X = normalize(cross(Y, Z));
            mat3 Rotate = mat3(X, Y, Z);
            Position = (Rotate * inPosition * uPushConstant.Scale) + uPushConstant.Origin;
            break;
        }
        case _SPRITE_TYPE_FACING_UP_RIGHT:
        {
            vec3 X = normalize(ubo.EyePosition - uPushConstant.Origin);
            vec3 Y = normalize(cross(vec3(0.0, 0.0, 1.0), X));
            vec3 Z = normalize(cross(X, Y));
            mat3 Rotate = mat3(X, Y, Z);
            Position = (Rotate * inPosition * uPushConstant.Scale) + uPushConstant.Origin;
            break;
        }
        case _SPRITE_TYPE_PARALLEL:
        {
            vec3 X = normalize(-ubo.EyeDirection);
            vec3 Y = normalize(cross(vec3(0.0, 0.0, 1.0), X));
            vec3 Z = normalize(cross(X, Y));
            mat3 Rotate = mat3(X, Y, Z);
            Position = (Rotate * inPosition * uPushConstant.Scale) + uPushConstant.Origin;
            break;
        }
        case _SPRITE_TYPE_ORIENTED:
        {
            // 关于实体角度angle = 倾斜 翻转 滚动 = pitch yaw roll
            // pitch [-90, 90] -90时朝上，90时朝下，绕y轴旋转
            // yaw [0, 360] 0时朝x轴，绕z轴旋转
            // roll [0, 360] 0时上方是z轴，绕x轴旋转
            Position = inPosition;
            Position.zx = rotate(Position.zx, uPushConstant.Angle.x);
            Position.xy = rotate(Position.xy, uPushConstant.Angle.y);
            Position.yz = rotate(Position.yz, uPushConstant.Angle.z);
            Position += uPushConstant.Origin;
            break;
        }
        case _SPRITE_TYPE_PARALLEL_ORIENTED:
        {
            vec3 TempPosition = inPosition;
            TempPosition.zx = rotate(TempPosition.zx, uPushConstant.Angle.x);
            TempPosition.xy = rotate(TempPosition.xy, uPushConstant.Angle.y);
            TempPosition.yz = rotate(TempPosition.yz, uPushConstant.Angle.z);
            vec3 X = normalize(-ubo.EyeDirection);
            vec3 Y = normalize(cross(vec3(0.0, 0.0, 1.0), X));
            vec3 Z = normalize(cross(X, Y));
            mat3 Rotate = mat3(X, Y, Z);
            Position = (Rotate * TempPosition * uPushConstant.Scale) + uPushConstant.Origin;
            break;
        }
        default:
        {
            Position = inPosition + uPushConstant.Origin;
            break;
        }
    }

    gl_Position = ubo.Proj * ubo.View * vec4(Position, 1.0);
    outFragTexCoord = inTexCoord;
}