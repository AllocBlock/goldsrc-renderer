#pragma once

#include <filesystem>

namespace Common
{
    float mod(float vVal, float vMax);
    std::vector<char> readFileAsChar(std::filesystem::path vFilePath);
}
