#include "IOLog.h"
#include <chrono>
#include <iomanip>

CIOLog* CIOLog::GlobalLogger = nullptr;

CIOLog::CIOLog(std::string vFileName)
{
    if (!vFileName.empty()) setFile(vFileName);
}
CIOLog::~CIOLog()
{
    if (m_OutStream.is_open()) m_OutStream.close();
}

bool CIOLog::isFileSet()
{
    return m_OutStream.is_open();
}

void CIOLog::setFile(std::string vFileName)
{
    if (m_OutStream && m_OutStream.is_open())
    {
        if (m_FileName == vFileName) return;
        else m_OutStream.close();
    }
    m_FileName = vFileName;
    m_OutStream.open(vFileName);
}

template <typename T>
bool CIOLog::log(T vData)
{
    if (!m_OutStream || !m_OutStream.is_open()) return false;
    m_OutStream << vData;
    return true;
}

std::ostream& CIOLog::logStream()
{
    return m_OutStream;
}

namespace GlobalLogger {
    bool isFileSet()
    {
        return CIOLog::GlobalLogger && CIOLog::GlobalLogger->isFileSet();
    }

    void setFile(std::string vFileName)
    {
        if (!CIOLog::GlobalLogger) CIOLog::GlobalLogger = new CIOLog(vFileName);
        else CIOLog::GlobalLogger->setFile(vFileName);
    }

    template <typename T>
    bool log(T vData)
    {
        logStream() << vData;
    }

    std::ostream& logStream()
    {
        if (!CIOLog::GlobalLogger) CIOLog::GlobalLogger = new CIOLog();

        std::ostream& Stream = CIOLog::GlobalLogger->logStream();
        auto Time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        tm LocalTime;
        localtime_s(&LocalTime, &Time);
        Stream << std::endl << std::put_time(&LocalTime, u8"%Y年%m月%d日 %H时%M分%S秒：") << std::endl;
        return Stream;
    }
}