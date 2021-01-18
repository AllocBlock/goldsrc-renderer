#pragma once

#include <string>
#include <fstream>
#include <filesystem>

class CIOLog
{
public:
    static CIOLog* GlobalLogger;
    CIOLog(std::filesystem::path vFilePath = "Events.log");
    ~CIOLog();

    bool isFileSet();
    void setFile(std::filesystem::path vFilePath);
    template <typename T> bool log(T vData);
    std::ostream& logStream();

private:
    std::filesystem::path m_FilePath;
    std::ofstream m_OutStream;
};

namespace GlobalLogger {
    bool isFileSet();
    void setFile(std::filesystem::path vFilePath);
    template <typename T> bool log(T vData);
    std::ostream& logStream();
}