#include "ImguiVullkan.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imfilebrowser.h"
#include <iostream>

SResultReadScene CImguiVullkan::readScene(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc)
{
    SResultReadScene Result;
    Result.Succeed = false;
    if (!std::filesystem::exists(vFilePath))
    {
        Result.Message = u8"�ļ������ڣ�" + vFilePath.u8string() + u8"��";
    }
    else if (vFilePath.extension() == ".bsp")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::readBspFile(vFilePath, vProgressReportFunc);
    }
    else if (vFilePath.extension() == ".map")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::readMapFile(vFilePath, vProgressReportFunc);
    }
    else if (vFilePath.extension() == ".obj")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::readObjFile(vFilePath, vProgressReportFunc);
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

    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    // �ļ�ѡ���
    static ImGui::FileBrowser FileDialog;
    FileDialog.Display();
    if (FileDialog.HasSelected())
    {
        static std::function<void(std::string)> ProgressReportFunc = [=](std::string vMessage)
        {
            m_LoadingProgressReport = vMessage;
        };

        m_LoadingFilePath = FileDialog.GetSelected(); FileDialog.ClearSelected();
        ImGui::OpenPopup(u8"��ʾ");
        m_FileReadingPromise = std::async(readScene, m_LoadingFilePath, ProgressReportFunc);
    }

    // �ļ����ؿ�
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"��ʾ", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(u8"�����ļ���...");
        ImGui::Text((u8"[ " + m_LoadingFilePath.u8string() + u8" ]").c_str());
        if (!m_LoadingProgressReport.empty()) ImGui::Text((u8"���ȣ�" + m_LoadingProgressReport).c_str());
        
        if (m_FileReadingPromise.valid() &&
            m_FileReadingPromise.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            ImGui::CloseCurrentPopup();
            m_LoadingFilePath = "";
            m_LoadingProgressReport = "";
            try
            {
                const SResultReadScene& ResultScene = m_FileReadingPromise.get();
                if (ResultScene.Succeed)
                    m_pRenderer->loadScene(ResultScene.Scene);
                else
                    showAlert(ResultScene.Message);
            }
            catch (std::exception& vException)
            {
                showAlert(vException.what());
            }
        }
        ImGui::EndPopup();
    }

    // �����
    GUIAlert.draw();
    
    ImGui::Begin(u8"����", NULL, ImGuiWindowFlags_MenuBar);
    // �˵���
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"�ļ�"))
        {
            if (ImGui::MenuItem(u8"��"))
            {
                FileDialog.SetTitle(u8"��");
                FileDialog.SetTypeFilters({ ".bsp", ".map", ".obj", ".*" });
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
        bool IgnoreAllAlert = GUIAlert.getIgnoreAll();
        ImGui::Checkbox(u8"�������о���", &IgnoreAllAlert);
        GUIAlert.setIgnoreAll(IgnoreAllAlert);
    }

    ImGui::End();

    // DEBUG
    if (ImGui::Button("test alert"))
    {
        showAlert("alert1");
        showAlert("alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2");
        showAlert("alert3");
    }

    ImGui::Render();
}