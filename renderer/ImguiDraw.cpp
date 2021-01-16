#include "ImguiVullkan.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuiFileDialogConfig.h"
#include "ImGuiFileDialog.h"
#include <iostream>

SResultReadScene CImguiVullkan::readScene(std::filesystem::path vFilePath)
{
    SResultReadScene Result;
    Result.Succeed = false;
    if (!std::filesystem::exists(vFilePath))
    {
        Result.Message = u8"文件不存在（" + vFilePath.u8string() + u8"）";
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
        Result.Message = u8"不支持的文件格式（" + vFilePath.extension().u8string() + u8"）";
    }
    return Result;
}

void CImguiVullkan::__drawGUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    // 弹出框
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            m_LoadingFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
            ImGuiFileDialog::Instance()->Close();
            ImGui::OpenPopup(u8"提示");
            m_FileReadingPromise = std::async(readScene, m_LoadingFilePath);
        }
    }

    // 文件加载框
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"提示", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(u8"加载文件中...");
        ImGui::Text((u8"[ " + m_LoadingFilePath.u8string() + u8" ]").c_str());
        
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

    // 警告框
    GUIAlert.draw();
    
    ImGui::Begin(u8"设置", NULL, ImGuiWindowFlags_MenuBar);
    // 菜单栏
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"文件"))
        {
            if (ImGui::MenuItem(u8"打开"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", u8"打开", ".map,.obj,.*", ".");
            }
            if (ImGui::MenuItem(u8"退出"))
            {
                glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // 帮助
    ImGui::Text(u8"操作方法");
    ImGui::BulletText(u8"按住空格开始控制相机，松开即结束");
    ImGui::BulletText(u8"WASD前后左右移动");
    ImGui::BulletText(u8"按住左Shift键加速，按住左Ctrl键减速");
    ImGui::BulletText(u8"移动鼠标转动视角");
    ImGui::Separator();

    // 相机设置
    if (ImGui::CollapsingHeader(u8"相机", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static float CameraSpeed = m_pInteractor->getSpeed();
        ImGui::SliderFloat(u8"速度", &CameraSpeed, 0.1f, 10.0f, "%.1f"); ImGui::SameLine();
        if (ImGui::Button(u8"重置")) CameraSpeed = 3.0f;
        m_pInteractor->setSpeed(CameraSpeed);

        static float Fov = m_pInteractor->getCamera()->getFov();
        ImGui::SliderFloat(u8"视野范围", &Fov, 10.0f, 170.0f, "%.1f"); ImGui::SameLine();
        if (ImGui::Button(u8"重置")) Fov = 120.0f;
        m_pInteractor->getCamera()->setFov(Fov);

        if (ImGui::Button(u8"重置相机"))
        {
            m_pInteractor->reset();
            CameraSpeed = m_pInteractor->getSpeed();
            Fov = m_pInteractor->getCamera()->getFov();
        }
    }

    if (ImGui::CollapsingHeader(u8"其他", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool IgnoreAllAlert = GUIAlert.getIgnoreAll();
        ImGui::Checkbox(u8"屏蔽所有警告", &IgnoreAllAlert);
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