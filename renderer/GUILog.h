#pragma once
#include <vector>
#include <string>
#include <istream>
#include <filesystem>

class CGUILog
{
public:
    void log(std::filesystem::path vText);
    void draw();
    friend std::istream& operator >> (std::istream& vIn, CGUILog& vGUILog);
private:
    std::string __getCurrentTime();

    bool m_Open = true;
    bool m_HasNewLog = false;
    std::vector<std::filesystem::path> m_Logs;
};

std::istream& operator >> (std::istream& vIn, CGUILog& vGUILog);

