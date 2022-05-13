#include "Log.h"

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