#include "ImguiMain.h"
#include "Common.h"
#include "SceneInterface.h"
#include "SceneCommon.h"
#include "ImguiRendererGoldSrc.h"
#include "ImguiRendererSimple.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imfilebrowser.h"

#include <iostream>
#include <set>
#include <future>

using namespace Common;

CGUIMain::CGUIMain(): m_pCamera(std::make_shared<CCamera>())
{
    static std::function<void(std::string)> ProgressReportFunc = [=](std::string vMessage)
    {
        m_LoadingProgressReport = vMessage;
    };

    Scene::setGlobalReportProgressFunc(ProgressReportFunc);

    static auto RequestFilePathFunc = [=](std::string vMessage, std::string vFilter) -> Scene::SRequestResultFilePath
    {
        m_FileSelection.setTitle(vMessage);
        m_FileSelection.setFilters({ vFilter });
        std::promise<std::filesystem::path> Promise;
        auto Future = Promise.get_future();
        m_FileSelection.start(std::move(Promise));
        Future.wait();
        auto Path = Future.get();
        if (Path.empty())
            return { Scene::ERequestResultState::CANCEL, "" };
        else
            return { Scene::ERequestResultState::CONTINUE, Path };
    };

    Scene::setGlobalRequestFilePathFunc(RequestFilePathFunc);
}

SResultReadScene CGUIMain::readScene(std::filesystem::path vFilePath)
{
    SResultReadScene Result;
    Result.Succeed = false;
    if (!std::filesystem::exists(vFilePath))
    {
        Result.Message = u8"文件不存在（" + vFilePath.u8string() + u8"）";
    }
    else if (vFilePath.extension() == ".bsp" ||
        vFilePath.extension() == ".rmf" ||
        vFilePath.extension() == ".map" ||
        vFilePath.extension() == ".bsp" ||
        vFilePath.extension() == ".obj" ||
        vFilePath.extension() == ".mdl"
        )
    {
        std::string Extension = vFilePath.extension().string().substr(1);
        Result.Succeed = true;
        Result.pScene = SceneReader::read(Extension, vFilePath);
    }
    else
    {
        Result.Message = u8"不支持的文件格式（" + vFilePath.extension().u8string() + u8"）";
    }
    return Result;
}

void CGUIMain::showAlert(std::string vText)
{
    m_GUIAlert.appendAlert(vText);
    Common::Log::log(u8"警告: " + vText);
}

void CGUIMain::log(std::string vText)
{
    m_GUILog.log(vText);
}

std::vector<VkCommandBuffer> CGUIMain::_requestCommandBuffersV(uint32_t vImageIndex)
{
    auto CommandBufferSet = CGUIBase::_requestCommandBuffersV(vImageIndex);

    if (m_pRenderer)
    {
        auto RendererCommandBufferSet = m_pRenderer->requestCommandBuffers(vImageIndex);
        CommandBufferSet.insert(CommandBufferSet.begin(), RendererCommandBufferSet.begin(), RendererCommandBufferSet.end());
    }

    return CommandBufferSet;
}

void CGUIMain::_initV()
{
    CGUIBase::_initV();

    Common::Log::setLogObserverFunc([=](std::string vText)
    {
        m_GUILog.log(vText);
    });

    __recreateRenderer();
}

void CGUIMain::_updateV(uint32_t vImageIndex)
{
    __drawGUI();
    if (m_pRenderer)
        m_pRenderer->update(vImageIndex);
}

void CGUIMain::_recreateV()
{
    CGUIBase::_recreateV();
    if (m_pRenderer)
        m_pRenderer->recreate(m_AppInfo.ImageFormat, m_AppInfo.Extent, m_AppInfo.TargetImageViewSet);
}

void CGUIMain::_destroyV()
{
    if (m_pRenderer)
        m_pRenderer->destroy();
    CGUIBase::_destroyV();
}

void CGUIMain::__recreateRenderer()
{
    vkDeviceWaitIdle(m_AppInfo.Device);
    if (m_pRenderer)
        m_pRenderer->destroy();

    switch (m_RenderMethod)
    {
    case ERenderMethod::DEFAULT:
    {
        m_pRenderer = std::make_shared<CRendererSceneSimple>();
        m_pRenderer->init(m_AppInfo, ERendererPos::BEGIN);
        m_pRenderer->setCamera(m_pCamera);
        m_pGuiRenderer = std::make_shared<CImguiRendererSimple>();
        m_pGuiRenderer->setTarget(m_pRenderer);
        break;
    }
    case ERenderMethod::BSP:
    {
        m_pRenderer = std::make_shared<CRendererSceneGoldSrc>();
        m_pRenderer->init(m_AppInfo, ERendererPos::BEGIN);
        m_pRenderer->setCamera(m_pCamera);
        m_pGuiRenderer = std::make_shared<CImguiRendererGoldSrc>();
        m_pGuiRenderer->setTarget(m_pRenderer);
        break;
    }
    default:
        break;
    }

    _ASSERTE(m_pInteractor);
    m_pInteractor->setRendererScene(m_pRenderer);

    if (m_pScene)
        m_pRenderer->loadScene(m_pScene);
}

void CGUIMain::__drawGUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    ImGui::ShowDemoWindow();
    // 文件选择框
    m_FileSelection.draw();
    static std::future<std::filesystem::path> FileSelectionFuture;
    if (FileSelectionFuture.valid() &&
        FileSelectionFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
    {
        auto Path = FileSelectionFuture.get();
        if (!Path.empty())
        {
            m_LoadingFilePath = Path;
            m_FileReadingFuture = std::async(readScene, m_LoadingFilePath);
        }
    }

    // 文件加载信息框
    if (!m_FileSelection.isOpen() && !m_LoadingFilePath.empty() && !ImGui::IsPopupOpen(u8"提示"))
        ImGui::OpenPopup(u8"提示"); // 打开加载提示
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"提示", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text(u8"加载文件中...");
        ImGui::Text((u8"[ " + m_LoadingFilePath.u8string() + u8" ]").c_str());
        if (!m_LoadingProgressReport.empty()) ImGui::Text((u8"进度：" + m_LoadingProgressReport).c_str());

        if (m_FileReadingFuture.valid() &&
            m_FileReadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            ImGui::CloseCurrentPopup();
            m_LoadingFilePath = "";
            m_LoadingProgressReport = "";
            const SResultReadScene& ResultScene = m_FileReadingFuture.get();
            if (ResultScene.Succeed)
            {
                m_pRenderer->loadScene(ResultScene.pScene);
                m_pScene = ResultScene.pScene;
            }
            else
                showAlert(ResultScene.Message);
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
                if (!m_FileSelection.isOpen())
                {
                    std::promise<std::filesystem::path> FileSelectionPromise;
                    FileSelectionFuture = FileSelectionPromise.get_future();
                    m_FileSelection.setTitle(u8"打开");
                    m_FileSelection.setFilters({ ".bsp", ".rmf", ".map", ".obj", ".mdl", ".*" });
                    m_FileSelection.start(std::move(FileSelectionPromise));
                }
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
        std::shared_ptr<CCamera> pCamera = m_pCamera;
        glm::vec3 Pos = pCamera->getPos();

        float FullWidth = ImGui::CalcItemWidth();
        ImGui::BeginGroup();
        ImGui::PushID(u8"相机位置坐标");
        ImGui::PushID(0); ImGui::SetNextItemWidth(FullWidth / 3);
        ImGui::DragFloat("", &Pos.x, 0.01f, 0.0f, 0.0f, "X: %.2f");
        ImGui::PopID(); ImGui::SameLine(); ImGui::PushID(1); ImGui::SetNextItemWidth(FullWidth / 3);
        ImGui::DragFloat("", &Pos.y, 0.01f, 0.0f, 0.0f, "Y: %.2f"); ImGui::SameLine();
        ImGui::PopID(); ImGui::SameLine(); ImGui::PushID(2); ImGui::SetNextItemWidth(FullWidth / 3);
        ImGui::DragFloat("", &Pos.z, 0.01f, 0.0f, 0.0f, "Z: %.2f");
        ImGui::PopID(); ImGui::PopID();
        ImGui::EndGroup(); ImGui::SameLine();
        ImGui::Text(u8"相机位置");
        pCamera->setPos(Pos);

        float Theta = static_cast<float>(pCamera->getTheta());
        float Phi = static_cast<float>(pCamera->getPhi());
        ImGui::BeginGroup();
        ImGui::PushID(u8"相机方向参数");
        ImGui::PushID(0); ImGui::SetNextItemWidth(FullWidth / 2);
        ImGui::DragFloat("", &Theta, 0.2f, 0.0f, 0.0f, u8"俯仰角: %.2f");
        ImGui::PopID(); ImGui::SameLine(); ImGui::PushID(1); ImGui::SetNextItemWidth(FullWidth / 2);
        ImGui::DragFloat("", &Phi, 0.2f, 0.0f, 0.0f, u8"环视角: %.2f");
        ImGui::PopID(); ImGui::PopID();
        ImGui::EndGroup(); ImGui::SameLine();
        ImGui::Text(u8"相机方向");
        pCamera->setTheta(Theta);
        pCamera->setPhi(Phi);

        static float CameraSpeed = m_pInteractor->getSpeed();
        ImGui::SliderFloat(u8"速度", &CameraSpeed, 0.1f, 10.0f, "%.1f"); ImGui::SameLine();
        if (ImGui::Button(u8"重置")) CameraSpeed = 3.0f;
        m_pInteractor->setSpeed(CameraSpeed);

        static float Fov = pCamera->getFov();
        ImGui::SliderFloat(u8"视野范围", &Fov, 10.0f, 170.0f, "%.1f"); ImGui::SameLine();
        if (ImGui::Button(u8"重置")) Fov = 120.0f;
        pCamera->setFov(Fov);

        if (ImGui::Button(u8"重置相机"))
        {
            m_pInteractor->reset();
            CameraSpeed = m_pInteractor->getSpeed();
            Fov = pCamera->getFov();
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

        static ERenderMethod LastMethod = m_RenderMethod;
        int RenderMethodIndex = static_cast<int>(std::find(RenderMethods.begin(), RenderMethods.end(), m_RenderMethod) - RenderMethods.begin());
        ImGui::Combo(u8"渲染模式", &RenderMethodIndex, RenderMethodNames.data(), static_cast<int>(RenderMethods.size()));
        m_RenderMethod = RenderMethods[RenderMethodIndex];
        if (LastMethod != m_RenderMethod)
        {
            LastMethod = m_RenderMethod;
            __recreateRenderer();
        }

        m_pGuiRenderer->draw();
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