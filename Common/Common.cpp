#include "Common.h"

#include <fstream>

const float g_ModOffset = 1e-9;

float Common::mod(float vVal, float vMax)
{
    if (vVal < 0)
    {
        int Times = std::ceil(std::abs(vVal) / vMax - g_ModOffset);
        return vVal + Times * vMax;
    }
    else
    {
        int Times = std::floor(vVal / vMax);
        return vVal - Times * vMax;
    }
}

std::vector<char> Common::readFileAsChar(std::filesystem::path vFilePath)
{
    if (!std::filesystem::exists(vFilePath))
    {
        std::filesystem::path FullPath = std::filesystem::absolute(vFilePath);
        throw std::runtime_error(u8"未找到文件：" + FullPath.u8string());
    }

    std::ifstream File(vFilePath, std::ios::ate | std::ios::binary);

    if (!File.is_open())
        throw std::runtime_error(u8"读取文件失败：" + vFilePath.u8string());

    size_t FileSize = static_cast<size_t>(File.tellg());
    std::vector<char> Buffer(FileSize);
    File.seekg(0);
    File.read(Buffer.data(), FileSize);
    File.close();

    return Buffer;
}