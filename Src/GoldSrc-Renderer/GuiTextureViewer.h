#pragma once
#include <vector>
#include <string>

class CGuiTextureViewer
{
public:
    void draw();
private:

    bool m_Open = true;
    std::vector<std::string> m_Logs;
};

