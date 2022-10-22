#include "GuiRequestPopupModal.h"
#include "InterfaceUI.h"

#include <string>

std::future<Scene::ERequestResultState> CGuiRequestPopupModal::show(std::string vTitle, std::string vDescription)
{
    _ASSERTE(!m_IsShow);
    m_IsShow = true;
    m_Title = vTitle;
    m_Description = vDescription;
    m_Promise = std::promise<Scene::ERequestResultState>();
    m_IsToOpen = true;
    return m_Promise.get_future();
}

void CGuiRequestPopupModal::close(Scene::ERequestResultState vState)
{
    _ASSERTE(m_IsShow);
    m_IsShow = false;
    m_Promise.set_value(vState);
}

void CGuiRequestPopupModal::draw()
{
    if (!m_IsShow) return;
    if (m_IsToOpen)
    {
        UI::openPopup(m_Title.data());
        m_IsToOpen = false;
    }

    glm::vec2 Center = UI::getDisplayCenter();
    UI::setNextWindowPos(Center, UI::ESetVariableCondition::APPEARING, glm::vec2(0.5f, 0.5f));
    if (UI::beginPopupModal(m_Title.data(), NULL, UI::EWindowFlag::ALWAYS_AUTO_RESIZE))
    {
        if (!m_Description.empty())
            UI::text(m_Description.c_str(), true);

        bool Close = false;
        if (UI::button(u8"寻找文件"))
        {
            __setValue(Scene::ERequestResultState::CONTINUE);
            Close = true;
        }
        UI::sameLine();
        if (UI::button(u8"取消"))
        {
            __setValue(Scene::ERequestResultState::CANCEL);
            Close = true;
        }
        UI::sameLine();
        if (UI::button(u8"跳过"))
        {
             __setValue(Scene::ERequestResultState::IGNORE_);
            Close = true;
        }
        UI::sameLine();
        if (UI::button(u8"重试"))
        {
             __setValue(Scene::ERequestResultState::RETRY);
            Close = true;
        }

        if (Close)
            UI::closeCurrentPopup();

        UI::endPopup();
    }
}

void CGuiRequestPopupModal::__setValue(Scene::ERequestResultState vState)
{
    close(vState);
}