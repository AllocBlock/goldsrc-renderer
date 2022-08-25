#include "GuiAlert.h"
#include "Gui.h"

void CGuiAlert::appendAlert(std::string vText)
{
    if (!m_IgnoreAll)
    {
        if (m_AlertTexts.empty())
            m_Open = true;
        m_AlertTexts.push(vText);
    }
}

void CGuiAlert::draw()
{
    if (m_Open)
    {
        UI::openPopup(u8"警告");
        m_Open = false;
    }
    glm::vec2 Center = UI::getDisplayCenter();
    UI::setNextWindowPos(Center, UI::ESetVariableCondition::APPEARING, glm::vec2(0.5f));
    if (UI::beginPopupModal(u8"警告", nullptr, UI::EWindowFlag::ALWAYS_AUTO_RESIZE))
    {
        UI::text(m_AlertTexts.front());
        static bool IgnoreAllAlert = false;
        UI::toggle(u8"屏蔽所有警告", IgnoreAllAlert);

        if (UI::button(u8"确认"))
        {
            if (!IgnoreAllAlert)
            {
                m_AlertTexts.pop();
            }
            if (IgnoreAllAlert)
            {
                IgnoreAllAlert = false;
                m_IgnoreAll = true;
                while (!m_AlertTexts.empty()) m_AlertTexts.pop();
            }
        }
        else if (!IgnoreAllAlert)
        {
            UI::sameLine();
            if (UI::button(u8"确认全部"))
            {
                while (!m_AlertTexts.empty()) m_AlertTexts.pop();
            }
        }
        if (m_AlertTexts.empty()) UI::closeCurrentPopup();
        UI::endPopup();
    }
}
