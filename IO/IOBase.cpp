#include "IOBase.h"

#include <algorithm>

CIOBase::CIOBase()
{
    if (!GlobalLogger::isFileSet()) GlobalLogger::setFile("log.txt");
}

CIOBase::CIOBase(std::string vFileName)
{
    if (!GlobalLogger::isFileSet()) GlobalLogger::setFile("log.txt");
    m_FileName = vFileName;
}

bool CIOBase::read(std::string vFileName)
{
	if (!vFileName.empty()) m_FileName = vFileName;
    return _readV(m_FileName);
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