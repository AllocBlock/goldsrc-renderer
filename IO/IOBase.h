#pragma once

#include "IOLog.h"

#include <string>
#include <fstream>

const size_t MAX_LINE_SIZE = 1024;

class CIOBase
{
public:
    CIOBase();
    CIOBase(std::string vFileName);
    virtual ~CIOBase() = default;
    bool read(std::string vFileName = "");

protected:
    virtual bool _readV(std::string vFileName) = 0;
    std::string m_FileName;
};