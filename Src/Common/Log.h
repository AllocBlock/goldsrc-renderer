#pragma once
#include <functional>
#include <string>

namespace Log
{
    using LogFunc = std::function<void(std::string)>;

    void setLogObserverFunc(LogFunc vLogFunc);
    void setEnablePrintWhenLog(bool vEnable);
    void log(std::string vText);
    void print(std::string vText, bool vEndOfLine = true);
}