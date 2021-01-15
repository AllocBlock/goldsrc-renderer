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

    ImGui::Begin(u8"设置", NULL, ImGuiWindowFlags_MenuBar);

    // 弹出框
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
            showAlert(u8"不支持的文件格式（." + FilePath.extension().u8string() + u8"）");;
        }
        m_pRenderer->setScene(Scene);
    }

    // 菜单栏
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"文件"))
        {
            if (ImGui::MenuItem(u8"打开"))
            {
                FileDialog.SetTitle(u8"打开");
                FileDialog.SetTypeFilters({ ".map", ".*" });
                FileDialog.Open();
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
        ImGui::Checkbox(u8"屏蔽所有警告", &m_IgnoreAllAlert);
    }

    ImGui::End();

    if (!m_IgnoreAllAlert && !m_AlertTexts.empty())
    {
        if (!m_AlertTexts.empty()) ImGui::OpenPopup(u8"警告");
        ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::BeginPopupModal(u8"警告", NULL, ImGuiWindowFlags_AlwaysAutoResize);

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
                m_IgnoreAllAlert = true;
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
        ImGui::EndPopup();
        if (m_AlertTexts.empty()) ImGui::CloseCurrentPopup();
    }

    ImGui::Render();
}