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
        UI::openPopup(u8"����");
        m_Open = false;
    }
    glm::vec2 Center = UI::getDisplayCenter();
    UI::setNextWindowPos(Center, UI::ESetVariableCondition::APPEARING, glm::vec2(0.5f));
    if (UI::beginPopupModal(u8"����", nullptr, UI::EWindowFlag::ALWAYS_AUTO_RESIZE))
    {
        UI::text(m_AlertTexts.front());
        static bool IgnoreAllAlert = false;
        UI::toggle(u8"�������о���", IgnoreAllAlert);

        if (UI::button(u8"ȷ��"))
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
            if (UI::button(u8"ȷ��ȫ��"))
            {
                while (!m_AlertTexts.empty()) m_AlertTexts.pop();
            }
        }
        if (m_AlertTexts.empty()) UI::closeCurrentPopup();
        UI::endPopup();
    }
}
