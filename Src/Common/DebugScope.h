#pragma once
#include <string>

namespace Debug
{
    class Scope
    {
    public:
        Scope() = delete;
        Scope(const std::string& vName);
        ~Scope();

    private:
        std::string m_Name;
    };

    void log(const std::string& vMessage);
}