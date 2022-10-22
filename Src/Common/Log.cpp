#include "Log.h"
#include <iostream>

using namespace Common;

Log::LogFunc g_LogFunc = nullptr;

void Log::setLogObserverFunc(Log::LogFunc vLogFunc)
{
    g_LogFunc = vLogFunc;
}

bool Log::log(std::string vText)
{
    if (g_LogFunc)
    {
        g_LogFunc(vText);
        return true;
    }
    else
        return false;
}

void Log::print(std::string vText, bool vEndOfLine)
{
    std::cout << vText << vEndOfLine ? "\n" : "";
}
