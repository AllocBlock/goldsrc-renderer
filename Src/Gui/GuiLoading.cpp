#include "GuiLoading.h"

#include "InterfaceGui.h"

const std::string& gPopupKey = u8"提示";

void CGuiLoading::update(const Common::SLoadingInfo& vLoadingInfo)
{
    m_LoadingInfo = vLoadingInfo;
    if (m_LoadingInfo.Progress.has_value())
    {
        m_LoadingInfo.Progress = std::min(1.0f, std::max(0.0f, m_LoadingInfo.Progress.value()));
    }

    if (!UI::isPopupOpen(gPopupKey))
    {
        UI::openPopup(gPopupKey);
    }
}

void CGuiLoading::end()
{
    if (UI::isPopupOpen(gPopupKey))
    {
        m_CloseAtNextFrame = true;
    }
}

void CGuiLoading::renderUI()
{
    glm::vec2 Center = UI::getDisplayCenter();

    UI::setNextWindowPos(Center, UI::ESetVariableCondition::APPEARING, glm::vec2(0.5f, 0.5f));
    if (UI::beginPopupModal(u8"提示", nullptr, UI::EWindowFlag::ALWAYS_AUTO_RESIZE))
    {
        if (m_CloseAtNextFrame)
        {
            UI::closeCurrentPopup();
            m_CloseAtNextFrame = false;
        }
        else
        {
            std::vector<std::string> FirstLineTexts;
            if (m_LoadingInfo.Progress.has_value())
                FirstLineTexts.push_back("[" + Common::toString(m_LoadingInfo.Progress.value() * 100, 2) + "]");
            if (!m_LoadingInfo.FileName.empty())
                FirstLineTexts.push_back(m_LoadingInfo.FileName.string());

            if (!FirstLineTexts.empty())
                UI::text(Common::joinString(FirstLineTexts));
            if (!m_LoadingInfo.Message.empty())
            {
                UI::text(m_LoadingInfo.Message);
            }
        }
        UI::endPopup();
    }
}
