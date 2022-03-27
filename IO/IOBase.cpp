#include "IOBase.h"

#include <algorithm>

CIOBase::CIOBase()
{
}

CIOBase::CIOBase(std::filesystem::path vFilePath) : m_FilePath(vFilePath)
{
}

bool CIOBase::read()
{
    if (m_FilePath.empty())
    {
        throw std::runtime_error(u8"未指定要读取的文件");
    }
    else if (!std::filesystem::exists(m_FilePath))
    {
        throw std::runtime_error(u8"文件不存在 " + m_FilePath.u8string());
    }
    return _readV(m_FilePath);
}

bool CIOBase::read(std::filesystem::path vFilePath)
{
	m_FilePath = vFilePath;
    return _readV(m_FilePath);
}

bool CIOBase::isWhiteSpace(char vChar)
{
    if (vChar == ' ' || vChar == '\n' || vChar == '\r' || vChar == '\t') return true;
    else return false;
}

std::string CIOBase::trimString(std::string vString)
{
	if (vString.empty()) return "";
	int StartIndex = 0, EndIndex = static_cast<int>(vString.length() - 1);

	while (StartIndex < vString.length() && isWhiteSpace(vString[StartIndex]))
	{
		StartIndex++;
	}
	while (EndIndex >= 0 && isWhiteSpace(vString[EndIndex]))
	{
		if (StartIndex > EndIndex) return "";
		EndIndex--;
	}
	return vString.substr(StartIndex, EndIndex - StartIndex + 1);
}

std::vector<std::string> CIOBase::splitString(std::string vString, char vSpliter)
{
    std::vector<std::string> Result;

    size_t StartLineIndex = 0;
    for (size_t i = 0; i < vString.length(); ++i)
    {
        if (vString[i] == vSpliter)
        {
            std::string Part = vString.substr(StartLineIndex, i - StartLineIndex);
            Result.emplace_back(Part);
            StartLineIndex = i + 1;
            while (i < vString.length() && vString[i] == vSpliter)
                ++i;
            --i;
        }
        else if (i == vString.length() - 1)
        {
            std::string Part = vString.substr(StartLineIndex);
            Result.emplace_back(Part);
        }
    }
    return Result;
}

std::string CIOBase::toUpperCase(std::string vString)
{
    std::transform(vString.begin(), vString.end(), vString.begin(), ::toupper);
    return vString;
}