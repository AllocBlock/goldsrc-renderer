#include "GuiAlert.h"
#include "InterfaceGui.h"

void CGuiAlert::add(const std::string& vText)
{
    if (m_IgnoreAll) return;

    if (m_AlertTexts.empty())
        m_OpenAtNextFrame = true;
    m_AlertTexts.push(vText);
}

void CGuiAlert::renderUI()
{
    if (m_IgnoreAll) return;

    if (m_OpenAtNextFrame)
    {
        UI::openPopup(u8"����");
        m_OpenAtNextFrame = false;
    }

    glm::vec2 Center = UI::getDisplayCenter();
    UI::setNextWindowPos(Center, UI::ESetVariableCondition::APPEARING, glm::vec2(0.5f));
    if (UI::beginPopupModal(u8"����", nullptr, UI::EWindowFlag::ALWAYS_AUTO_RESIZE))
    {
        UI::text(m_AlertTexts.front());
        UI::toggle(u8"�������о���", m_IgnoreAllRealtime);

        if (UI::button(u8"ȷ��"))
        {
            m_AlertTexts.pop();
            if (m_IgnoreAllRealtime)
            {
                m_IgnoreAll = true;
                m_AlertTexts = {};
            }
        }
        else if (!m_IgnoreAllRealtime)
        {
            UI::sameLine();
            if (UI::button(u8"ȷ��ȫ��"))
            {
                m_AlertTexts = {};
            }
        }
        if (m_IgnoreAll || m_AlertTexts.empty()) UI::closeCurrentPopup();
        UI::endPopup();
    }
}
