#pragma once
#include <functional>
#include <string>

//#define _ENABLE_HANDLE_CREATION_LOG

namespace Log
{
    using LogFunc = std::function<void(const std::string&)>;

    void setLogObserverFunc(LogFunc vLogFunc);
    void setEnablePrintWhenLog(bool vEnable);
    void log(const std::string& vText);
    void print(const std::string& vText, bool vEndOfLine = true);
    
    void logCreation(const std::string& vType, uint64_t vHandle);
    void logCreation(const std::string& vType, const std::vector<uint64_t>& vHandleSet);
}