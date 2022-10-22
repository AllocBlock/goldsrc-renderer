#include "Log.h"
#include <iostream>

namespace
{
    Log::LogFunc gLogFunc = nullptr;
    bool gEnablePrintWhenLog = true;
}

void Log::setLogObserverFunc(Log::LogFunc vLogFunc)
{
    gLogFunc = vLogFunc;
}

// TIPS: default it's on
void setEnablePrintWhenLog(bool vEnable)
{
    gEnablePrintWhenLog = vEnable;
}

void Log::log(std::string vText)
{
    if (gEnablePrintWhenLog)
        print(vText);
    if (gLogFunc)
        gLogFunc(vText);
}

void Log::print(std::string vText, bool vEndOfLine)
{
    std::cout << vText << vEndOfLine ? "\n" : "";
}
