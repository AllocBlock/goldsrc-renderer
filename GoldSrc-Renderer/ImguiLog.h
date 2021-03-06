#pragma once
#include <vector>
#include <string>
#include <istream>

class CImguiLog
{
public:
    void log(std::string vText);
    void draw();
    friend std::istream& operator >> (std::istream& vIn, CImguiLog& vGUILog);
private:
    std::string __getCurrentTime();

    bool m_Open = true;
    bool m_HasNewLog = false;
    std::vector<std::string> m_Logs;
};

std::istream& operator >> (std::istream& vIn, CImguiLog& vGUILog);

