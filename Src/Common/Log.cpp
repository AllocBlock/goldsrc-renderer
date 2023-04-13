#include "Log.h"

#include <iostream>
#include <string>
#include <map>

namespace
{
    Log::LogFunc gLogFunc = nullptr;
    bool gEnablePrintWhenLog = true;
    std::map<std::string, size_t> gHandleCreatedCountMap;
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

void Log::log(const std::string& vText)
{
    if (gEnablePrintWhenLog)
        print(vText);
    if (gLogFunc)
        gLogFunc(vText);
}

void Log::print(const std::string& vText, bool vEndOfLine)
{
    std::cout << vText << (vEndOfLine ? "\n" : "");
}

void Log::logCreation(const std::string& vType, uint64_t vHandle)
{
#ifdef _ENABLE_HANDLE_CREATION_LOG
    if (gHandleCreatedCountMap.find(vType) == gHandleCreatedCountMap.end())
    {
        gHandleCreatedCountMap[vType] = 0;
    }
    std::cout << "create " + vType + " [" << gHandleCreatedCountMap[vType] << "] = 0x" << std::setbase(16) << vHandle << std::setbase(10) << std::endl;
    gHandleCreatedCountMap[vType]++;
#endif
}

void Log::logCreation(const std::string& vType, const std::vector<uint64_t>& vHandleSet)
{
#ifdef _ENABLE_HANDLE_CREATION_LOG
    if (gHandleCreatedCountMap.find(vType) == gHandleCreatedCountMap.end())
    {
        gHandleCreatedCountMap[vType] = 0;
    }
    std::cout << "create " + vType + "s:\n";
    for (auto Handle : vHandleSet)
    {
        std::cout << "\t[" << gHandleCreatedCountMap[vType] << "] = 0x" << std::setbase(16) << Handle << std::setbase(10) << "\n";
        gHandleCreatedCountMap[vType]++;
    }
#endif
}