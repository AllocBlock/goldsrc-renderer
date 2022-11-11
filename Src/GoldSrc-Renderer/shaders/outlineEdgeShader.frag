#version 450

layout(location = 0) in vec2 inFragTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    uvec2 ImageExtent;
} ubo;
layout(binding = 1) uniform sampler2D uTexInput;

layout(location = 0) out vec4 outColor;

bool isInScreen(vec2 uv)
{
    return uv.x >= 0.0 && uv.x < 1.0 && uv.y >= 0.0 && uv.y < 1.0;
}

void main()
{
    const int N = 5;
    const int MaxNum = N * N;
    const vec2 PixelSize = vec2(1.0) / vec2(ubo.ImageExtent);
    int Num = 0;
    
    // for (int i = 0; i < MaxNum; ++i)
    // {
    //     vec2 Shift = (vec2(i % N, i / N) - N/2) * PixelSize;
    //     bool hasValue = (texture(uTexInput, inFragTexCoord + Shift).x > 0.5);
    //     if (hasValue) Num++;
    // }

    const int Half = N / 2;
    for (int i = -Half; i <= Half; ++i)
    {
        for (int k = -Half; k <= Half; ++k)
        {
            vec2 uv = inFragTexCoord + vec2(i, k) * PixelSize;
            if (!isInScreen(uv)) continue;
            bool hasValue = (texture(uTexInput, uv).x > 0.5);
            if (hasValue) Num++;
        }
    }

    bool IsEdge = (Num > 0 && Num < MaxNum);
    if (!IsEdge)
        discard;
    outColor = vec4(1.0, 0.2, 0.2, 0.8);
}