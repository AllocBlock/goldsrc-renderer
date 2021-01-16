#include "ImguiVullkan.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuiFileDialogConfig.h"
#include "ImGuiFileDialog.h"

SResultReadScene CImguiVullkan::readScene(std::filesystem::path vFilePath)
{
    SResultReadScene Result;
    Result.Succeed = false;
    if (!std::filesystem::exists(vFilePath))
    {
        Result.Message = u8"�ļ������ڣ�" + vFilePath.u8string() + u8"��";
    }
    else if (vFilePath.extension() == ".map")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::readMapFile(vFilePath.string());
    }
    else if (vFilePath.extension() == ".obj")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::readObjFile(vFilePath.string());
    }
    else
    {
        Result.Message = u8"��֧�ֵ��ļ���ʽ��" + vFilePath.extension().u8string() + u8"��";
    }
    return Result;
}

void CImguiVullkan::__drawGUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ������
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            m_LoadingFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            ImGuiFileDialog::Instance()->Close();
            ImGui::OpenPopup(u8"�ļ�������ʾ");
            m_FileReadingPromise = std::async(readScene, m_LoadingFilePath);
        }
    }

    // �ļ����ؿ�
    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"�ļ�������ʾ", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(u8"�����ļ���...");
        ImGui::Text((u8"[" + m_LoadingFilePath.u8string() + u8"]").c_str());
        
        if (m_FileReadingPromise.valid() &&
            m_FileReadingPromise.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            ImGui::CloseCurrentPopup();
            m_LoadingFilePath = "";
            const SResultReadScene& ResultScene = m_FileReadingPromise.get();
            if (ResultScene.Succeed)
                m_pRenderer->loadScene(ResultScene.Scene);
            else
                showAlert(ResultScene.Message);
        }
        ImGui::EndPopup();
    }

    // �����
    if (!m_IgnoreAllAlert && !m_AlertTexts.empty())
    {
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
        if (m_AlertTexts.empty()) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::Begin(u8"����", NULL, ImGuiWindowFlags_MenuBar);
    // �˵���
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"�ļ�"))
        {
            if (ImGui::MenuItem(u8"��"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", u8"��", ".map,.obj,.*", ".");
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

    ImGui::Render();
}