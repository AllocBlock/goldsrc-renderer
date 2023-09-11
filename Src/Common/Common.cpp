#include "Common.h"

#include <fstream>
#include <sstream>

const float g_ModOffset = 1e-9f;

float Common::mod(float vVal, float vMax)
{
    if (vVal < 0)
    {
        int Times = static_cast<int>(std::ceil(std::abs(vVal) / vMax - g_ModOffset));
        return vVal + Times * vMax;
    }
    else
    {
        int Times = static_cast<int>(std::floor(vVal / vMax));
        return vVal - Times * vMax;
    }
}

std::vector<uint8_t> Common::readFileAsByte(const std::filesystem::path& vFilePath)
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
    std::vector<uint8_t> Buffer(FileSize);
    File.seekg(0);
    File.read(reinterpret_cast<char*>(Buffer.data()), FileSize);
    File.close();

    return Buffer;
}

std::string Common::readFileAsString(const std::filesystem::path& vFilePath)
{
    if (!std::filesystem::exists(vFilePath))
    {
        std::filesystem::path FullPath = std::filesystem::absolute(vFilePath);
        throw std::runtime_error(u8"未找到文件：" + FullPath.u8string());
    }

    std::ifstream File(vFilePath);
    return { std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>() };
}

void Common::writeStringToFile(const std::string& Data, const std::filesystem::path& vFilePath)
{
    std::ofstream File(vFilePath);
    File << Data;
    File.close();
}

std::vector<std::string> Common::split(const std::string& vStr, const std::string& vSplitter)
{
    size_t CurPos = 0, LastPos = 0;
    std::vector<std::string> ResultSet;
    std::string token;
    while (CurPos != std::string::npos) {
        CurPos = vStr.find(vSplitter, LastPos);
        ResultSet.emplace_back(vStr.substr(LastPos, CurPos - LastPos));
        LastPos = CurPos + vSplitter.length();
    }
    return ResultSet;
}

bool __isWhiteSpace(char vChar)
{
    if (vChar == ' ') return true;
    if (vChar == '\t') return true;
    if (vChar == '\n') return true;
    return false;
}

std::string Common::trim(const std::string& vStr)
{
    if (vStr.empty()) return "";
    size_t Start = 0;
    size_t End = vStr.size();
    while (__isWhiteSpace(vStr.at(Start)))
        Start++;
    while (End > Start && __isWhiteSpace(vStr.at(End - 1)))
        End--;
    if (Start == End) return "";
    return vStr.substr(Start, End);
}

std::string Common::joinString(const std::vector<std::string>& vStrings, const std::string& vSeparator)
{
    std::string Result;
    for (size_t i = 0; i < vStrings.size(); ++i)
    {
        Result += (i == 0 ? "" : vSeparator) + vStrings[i];
    }
    return Result;
}

std::string Common::toString(double vValue, int roundDecimalPlace)
{
    _ASSERTE(roundDecimalPlace >= 1);
    std::stringstream stream;
    stream << std::fixed << std::setprecision(roundDecimalPlace) << vValue;
    return stream.str();
}
