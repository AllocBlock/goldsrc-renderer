#pragma once
#include <functional>
#include <string>

namespace Common
{
    namespace Log
    {
        using LogFunc = std::function<void(std::string)>;

        void setLogObserverFunc(LogFunc vLogFunc);
        bool log(std::string vText);
        void print(std::string vText, bool vEndOfLine = true);
    }
}