#pragma once

#include <filesystem>

namespace Common
{
    float mod(float vVal, float vMax);
    std::vector<char> readFileAsChar(std::filesystem::path vFilePath);

    template <typename T>
    T lerp(T vA, T vB, float vFactor)
    {
        return static_cast<T>(vA * vFactor + vB * (1 - vFactor));
    }
}
