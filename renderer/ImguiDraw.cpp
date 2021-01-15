#include "ImguiVullkan.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imfilebrowser.h"

void CImguiVullkan::__drawGUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Begin(u8"����", NULL, ImGuiWindowFlags_MenuBar);

    // ������
    static ImGui::FileBrowser FileDialog;
    FileDialog.Display();

    if (FileDialog.HasSelected())
    {
        std::filesystem::path FilePath = FileDialog.GetSelected(); FileDialog.ClearSelected();
        SScene Scene;
        if (FilePath.extension() == "map")
        {
            Scene = SceneReader::readMapFile(FileDialog.GetSelected().string());
        }
        else if (FilePath.extension() == "obj")
        {
            Scene = SceneReader::readMapFile(FileDialog.GetSelected().string());
        }
        else
        {
            showAlert(u8"��֧�ֵ��ļ���ʽ��." + FilePath.extension().u8string() + u8"��");;
        }
        m_pRenderer->setScene(Scene);
    }

    // �˵���
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"�ļ�"))
        {
            if (ImGui::MenuItem(u8"��"))
            {
                FileDialog.SetTitle(u8"��");
                FileDialog.SetTypeFilters({ ".map", ".*" });
                FileDialog.Open();
            }
            if (ImGui::MenuItem(u8"�˳�"))
            {
                glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // ����
    ImGui::Text(u8"��������");
    ImGui::BulletText(u8"��ס�ո�ʼ����������ɿ�������");
    ImGui::BulletText(u8"WASDǰ�������ƶ�");
    ImGui::BulletText(u8"��ס��Shift�����٣���ס��Ctrl������");
    ImGui::BulletText(u8"�ƶ����ת���ӽ�");
    ImGui::Separator();

    // �������
    if (ImGui::CollapsingHeader(u8"���", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static float CameraSpeed = m_pInteractor->getSpeed();
        ImGui::SliderFloat(u8"�ٶ�", &CameraSpeed, 0.1f, 10.0f, "%.1f"); ImGui::SameLine();
        if (ImGui::Button(u8"����")) CameraSpeed = 3.0f;
        m_pInteractor->setSpeed(CameraSpeed);

        static float Fov = m_pInteractor->getCamera()->getFov();
        ImGui::SliderFloat(u8"��Ұ��Χ", &Fov, 10.0f, 170.0f, "%.1f"); ImGui::SameLine();
        if (ImGui::Button(u8"����")) Fov = 120.0f;
        m_pInteractor->getCamera()->setFov(Fov);

        if (ImGui::Button(u8"�������"))
        {
            m_pInteractor->reset();
            CameraSpeed = m_pInteractor->getSpeed();
            Fov = m_pInteractor->getCamera()->getFov();
        }
    }

    if (ImGui::CollapsingHeader(u8"����", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox(u8"�������о���", &m_IgnoreAllAlert);
    }

    ImGui::End();

    if (!m_IgnoreAllAlert && !m_AlertTexts.empty())
    {
        if (!m_AlertTexts.empty()) ImGui::OpenPopup(u8"����");
        ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::BeginPopupModal(u8"����", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text(m_AlertTexts.front().c_str());
        static bool IgnoreAllAlert = false;
        ImGui::Checkbox(u8"�������о���", &IgnoreAllAlert);

        if (ImGui::Button(u8"ȷ��"))
        {
            if (!IgnoreAllAlert)
            {
                m_AlertTexts.pop();
            }
            if (IgnoreAllAlert)
            {
                m_IgnoreAllAlert = true;
                while (!m_AlertTexts.empty()) m_AlertTexts.pop();
            }
        }
        else if (!IgnoreAllAlert)
        {
            ImGui::SameLine();
            if (ImGui::Button(u8"ȷ��ȫ��"))
            {
                while (!m_AlertTexts.empty()) m_AlertTexts.pop();
            }
        }
        ImGui::EndPopup();
        if (m_AlertTexts.empty()) ImGui::CloseCurrentPopup();
    }

    ImGui::Render();
}