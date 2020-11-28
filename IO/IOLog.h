#pragma once

#include <string>
#include <fstream>

class CIOLog
{
public:
    static CIOLog* GlobalLogger;
    CIOLog(std::string vFileName = "");
    ~CIOLog();

    bool isFileSet();
    void setFile(std::string vFileName);
    template <typename T> bool log(T vData);
    std::ostream& logStream();

private:
    std::string m_FileName;
    std::ofstream m_OutStream;
};

namespace GlobalLogger {
    bool isFileSet();
    void setFile(std::string vFileName);
    template <typename T> bool log(T vData);
    std::ostream& logStream();
}