#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    uvec2 ImageExtent;
    int FilterSize;
} ubo;
layout(binding = 1) uniform sampler2D uTexInput;

layout(location = 0) out vec4 outColor;

bool isInScreen(vec2 uv)
{
    return uv.x >= 0.0 && uv.x < 1.0 && uv.y >= 0.0 && uv.y < 1.0;
}

void main()
{
    const int N = ubo.FilterSize;
    const vec2 PixelSize = vec2(1.0) / vec2(ubo.ImageExtent);
    
    int Num = 0;
    vec3 SumColor = vec3(0.0, 0.0, 0.0);
    const int Half = N / 2;
    for (int i = -Half; i <= Half; ++i)
    {
        for (int k = -Half; k <= Half; ++k)
        {
            vec2 uv = inFragTexCoord + vec2(i, k) * PixelSize;
            if (!isInScreen(uv)) continue;
            vec3 color = texture(uTexInput, uv).rgb;
            Num++;
            SumColor += color;
        }
    }

    outColor = vec4(SumColor / Num, 1.0);
}