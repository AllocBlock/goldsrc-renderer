#pragma once
#include <vector>
#include <string>
#include <istream>

class CGUILog
{
public:
    void log(std::string vText);
    void draw();
    friend std::istream& operator >> (std::istream& vIn, CGUILog& vGUILog);
private:
    std::string __getCurrentTime();

    bool m_Open = true;
    std::vector<std::string> m_Logs;
};

std::istream& operator >> (std::istream& vIn, CGUILog& vGUILog);

