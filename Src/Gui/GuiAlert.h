#pragma once
#include "Common.h"

#include <string>
#include <queue>

class CGuiAlert
{
public:
    void add(const std::string& vText);
    void renderUI();

    _DEFINE_GETTER_SETTER(IgnoreAll, bool);

private:
    bool m_OpenAtNextFrame = false;
    bool m_IgnoreAll = false;
    bool m_IgnoreAllRealtime = false;
    std::queue<std::string> m_AlertTexts;
};