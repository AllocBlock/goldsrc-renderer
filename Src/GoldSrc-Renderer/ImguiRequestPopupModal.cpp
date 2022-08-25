#include "ImguiRequestPopupModal.h"
#include "NativeSystem.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <string>

std::future<Common::Scene::ERequestResultState> CImguiRequestPopupModal::show(std::string vTitle, std::string vDescription)
{
    _ASSERTE(!m_IsShow);
    m_IsShow = true;
    m_Title = vTitle;
    m_Description = vDescription;
    m_Promise = std::promise<Common::Scene::ERequestResultState>();
    m_IsToOpen = true;
    return m_Promise.get_future();
}

void CImguiRequestPopupModal::close(Common::Scene::ERequestResultState vState)
{
    _ASSERTE(m_IsShow);
    m_IsShow = false;
    m_Promise.set_value(vState);
}

void CImguiRequestPopupModal::draw()
{
    if (!m_IsShow) return;
    if (m_IsToOpen)
    {
        ImGui::OpenPopup(m_Title.data());
        m_IsToOpen = false;
    }

    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(m_Title.data(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (!m_Description.empty())
            ImGui::TextWrapped(m_Description.c_str());

        static bool IgnoreAllAlert = false;
        ImGui::Checkbox(u8"屏蔽所有警告", &IgnoreAllAlert);

        bool Close = false;
        if (ImGui::Button(u8"寻找文件"))
        {
            __setValue(Common::Scene::ERequestResultState::CONTINUE);
            Close = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"取消"))
        {
            __setValue(Common::Scene::ERequestResultState::CANCEL);
            Close = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"跳过"))
        {
             __setValue(Common::Scene::ERequestResultState::IGNORE_);
            Close = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(u8"重试"))
        {
             __setValue(Common::Scene::ERequestResultState::RETRY);
            Close = true;
        }

        if (Close)
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void CImguiRequestPopupModal::__setValue(Common::Scene::ERequestResultState vState)
{
    close(vState);
}