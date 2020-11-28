#include "IOBase.h"

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
    return _readV(vFileName);
}