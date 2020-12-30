#pragma once

#include "IOLog.h"

#include <string>
#include <vector>
#include <fstream>

const size_t MAX_LINE_SIZE = 1024;

class CIOBase
{
public:
    CIOBase();
    CIOBase(std::string vFileName);
    virtual ~CIOBase() = default;
    bool read(std::string vFileName = "");

    static bool isWhiteSpace(char vChar);
    static std::string trimString(std::string vString);
    static std::vector<std::string> splitString(std::string vString, char vSpliter = ' ');

protected:
    virtual bool _readV(std::string vFileName) = 0;
    
    std::string m_FileName;
};