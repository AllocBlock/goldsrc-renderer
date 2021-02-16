#include "ImguiVullkan.h"
#include "SceneInterface.h"

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
        Result.Message = u8"文件不存在（" + vFilePath.u8string() + u8"）";
    }
    else if (vFilePath.extension() == ".bsp")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::read("bsp", vFilePath, vProgressReportFunc);
    }
    else if (vFilePath.extension() == ".map")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::read("map", vFilePath, vProgressReportFunc);
    }
    else if (vFilePath.extension() == ".obj")
    {
        Result.Succeed = true;
        Result.Scene = SceneReader::read("obj", vFilePath, vProgressReportFunc);
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

    ImGui::ShowDemoWindow();
    // 文件选择框
    static ImGui::FileBrowser FileDialog;
    FileDialog.Display();
    if (FileDialog.HasSelected())
    {
        static std::function<void(std::string)> ProgressReportFunc = [=](std::string vMessage)
        {
            m_LoadingProgressReport = vMessage;
        };

        m_LoadingFilePath = FileDialog.GetSelected(); FileDialog.ClearSelected();
        ImGui::OpenPopup(u8"提示");
        m_FileReadingPromise = std::async(readScene, m_LoadingFilePath, ProgressReportFunc);
    }

    // 文件加载框
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"提示", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(u8"加载文件中...");
        ImGui::Text((u8"[ " + m_LoadingFilePath.u8string() + u8" ]").c_str());
        if (!m_LoadingProgressReport.empty()) ImGui::Text((u8"进度：" + m_LoadingProgressReport).c_str());
        
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

    // 警告框
    m_GUIAlert.draw();
    
    ImGui::Begin(u8"设置", NULL, ImGuiWindowFlags_MenuBar);
    // 菜单栏
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"文件"))
        {
            if (ImGui::MenuItem(u8"打开"))
            {
                FileDialog.SetTitle(u8"打开");
                FileDialog.SetTypeFilters({ ".bsp", ".map", ".obj", ".*" });
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

    // 渲染设置
    if (ImGui::CollapsingHeader(u8"渲染", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static const std::vector<ERenderMethod> RenderMethods =
        {
            ERenderMethod::DEFAULT,
            ERenderMethod::BSP
        };

        static const std::vector<const char*> RenderMethodNames =
        {
            u8"默认",
            u8"BSP树渲染"
        };


        static ERenderMethod LastMethod = m_pRenderer->getRenderMethod();
        int RenderMethodIndex = std::find(RenderMethods.begin(), RenderMethods.end(), m_pRenderer->getRenderMethod()) - RenderMethods.begin();
        if (LastMethod != RenderMethods[RenderMethodIndex])
        {
            LastMethod = RenderMethods[RenderMethodIndex];
            m_pRenderer->rerecordCommand();
        }
        ImGui::Combo(u8"渲染模式", &RenderMethodIndex, RenderMethodNames.data(), RenderMethods.size());
        m_pRenderer->setRenderMethod(RenderMethods[RenderMethodIndex]);

        glm::vec3 CameraPos = m_pRenderer->getCamera()->getPos();
        ImGui::Text((u8"相机位置：(" + std::to_string(CameraPos.x) + ", " + std::to_string(CameraPos.y) + ", " + std::to_string(CameraPos.z) + ")").c_str());
        std::optional<uint32_t> CameraNodeIndex = m_pRenderer->getCameraNodeIndex();
        if (CameraNodeIndex == std::nullopt)
            ImGui::Text(u8"相机所处节点：-");
        else
            ImGui::Text((u8"相机所处节点：" + std::to_string(CameraNodeIndex.value())).c_str());

        size_t VisableObjectNum = m_pRenderer->getRenderedObjectNum();
        ImGui::Text((u8"渲染物体数：" + std::to_string(VisableObjectNum)).c_str());

        std::vector<uint32_t> RenderNodeList = m_pRenderer->getRenderNodeList();
        if (!RenderNodeList.empty())
        {
            std::string RenderNodeListStr = "";
            for (uint32_t NodeIndex : RenderNodeList)
            {
                RenderNodeListStr += std::to_string(NodeIndex) + ", ";
            }
            ImGui::TextWrapped((u8"渲染节点：" + RenderNodeListStr).c_str());
        }

        static bool Culling = m_pRenderer->getCullingState();
        ImGui::Checkbox(u8"开启剔除", &Culling);
        m_pRenderer->setCullingState(Culling);

        if (Culling)
        {
            ImGui::BeginGroup();
            static bool FrustumCulling = m_pRenderer->getFrustumCullingState();
            ImGui::Checkbox(u8"CPU视锥剔除", &FrustumCulling);
            if (FrustumCulling != m_pRenderer->getFrustumCullingState()) m_pRenderer->rerecordCommand();
            m_pRenderer->setFrustumCullingState(FrustumCulling);

            static bool PVS = m_pRenderer->getPVSState();
            ImGui::Checkbox(u8"PVS剔除", &PVS);
            m_pRenderer->setPVSState(PVS);
            ImGui::EndGroup();
        }
    }

    if (ImGui::CollapsingHeader(u8"其他", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool IgnoreAllAlert = m_GUIAlert.getIgnoreAll();
        ImGui::Checkbox(u8"屏蔽所有警告", &IgnoreAllAlert);
        m_GUIAlert.setIgnoreAll(IgnoreAllAlert);
    }

    ImGui::End();

    // 帧率
    m_GUIFrameRate.draw();

    // 日志
    m_GUILog.draw();

    // DEBUG
    if (ImGui::Button("test alert"))
    {
        showAlert("alert1");
        showAlert("alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2");
        showAlert("alert3");
    }
    if (ImGui::Button("test log"))
    {
        log("log1");
        log("log2log2log2log2log2log2log2log2log2log2log2log2log2log2log2log2log2");
        log("log3");
    }

    ImGui::Render();
}