#include "CommonLog.h"

LogFunc g_LogFunc = nullptr;

void setGlobalLogFunc(LogFunc vLogFunc)
{
    g_LogFunc = vLogFunc;
}

bool globalLog(std::string vText)
{
    if (g_LogFunc)
    {
        g_LogFunc(vText);
        return true;
    }
    else
        return false;
}