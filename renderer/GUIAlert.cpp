#include "GUIAlert.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void CGUIAlert::appendAlert(std::string vText)
{
    if (!m_IgnoreAll)
    {
        if (m_AlertTexts.empty())
            m_Open = true;
        m_AlertTexts.push(vText);
    }
}

void CGUIAlert::draw()
{
    if (m_Open)
    {
        ImGui::OpenPopup(u8"警告");
        m_Open = false;
    }
    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"警告", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(m_AlertTexts.front().c_str());
        static bool IgnoreAllAlert = false;
        ImGui::Checkbox(u8"屏蔽所有警告", &IgnoreAllAlert);

        if (ImGui::Button(u8"确认"))
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
            ImGui::SameLine();
            if (ImGui::Button(u8"确认全部"))
            {
                while (!m_AlertTexts.empty()) m_AlertTexts.pop();
            }
        }
        if (m_AlertTexts.empty()) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}
