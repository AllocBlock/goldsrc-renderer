#pragma once
#include <string>

class CObject
{
public:
    void setObjectName(std::string vName) { m_Name = vName; }
    std::string getObjectName() { return m_Name; }
    void setMark(std::string vMark) { m_Mark = vMark; }
    std::string getMark() { return m_Mark; }

protected:
    std::string m_Name;
    std::string m_Mark;
};

