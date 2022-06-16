#pragma once
#include <string>
#include <queue>

class CGuiAlert
{
public:
    void appendAlert(std::string vText);
    void draw();
    bool getIgnoreAll() { return m_IgnoreAll; }
    void setIgnoreAll(bool vIgnoreAll) { m_IgnoreAll = vIgnoreAll; }

private:
    bool m_Open = false;
    bool m_IgnoreAll = false;
    std::queue<std::string> m_AlertTexts;
};

