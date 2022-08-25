#include "ImguiMain.h"
#include "Common.h"
#include "SceneInterface.h"
#include "SceneCommon.h"
#include "UserInterface.h"
#include "NativeSystem.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <iostream>
#include <set>
#include <future>
#include "SceneObjWriter.h"

using namespace Common;

CGUIMain::CGUIMain()
{
    Common::Log::setLogObserverFunc([=](std::string vText)
    {
        m_GUILog.log(vText);
    });

    static std::function<void(std::string)> ProgressReportFunc = [=](std::string vMessage)
    {
        m_LoadingProgressReport = vMessage;
    };

    Scene::setGlobalReportProgressFunc(ProgressReportFunc);

    static auto RequestFilePathFunc = [=](std::string vMessage, std::string vFilter) -> Scene::SRequestResultFilePath
    {
        auto Future = m_RequestPopupModal.show(u8"未找到文件", vMessage);
        Scene::ERequestResultState ActionResult = Future.get();
        if (ActionResult == Scene::ERequestResultState::CONTINUE)
        {
            auto SelectResult = NativeSystem::createOpenFileDialog(vFilter);
            if (SelectResult)
                return { Scene::ERequestResultState::CONTINUE, SelectResult.FilePath };
            else
                return { Scene::ERequestResultState::CANCEL, "" };
        }
        else
            return { ActionResult, "" };
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

void CGUIMain::_renderUIV()
{
    ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

    // 文件加载信息框
    if (!m_RequestPopupModal.isShow() && !m_LoadingFilePath.empty() && !ImGui::IsPopupOpen(u8"提示"))
        ImGui::OpenPopup(u8"提示"); // 打开加载提示
    ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(u8"提示", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        UI::text(u8"加载文件中...");
        UI::text((u8"[ " + m_LoadingFilePath.u8string() + u8" ]").c_str());
        if (!m_LoadingProgressReport.empty()) UI::text((u8"进度：" + m_LoadingProgressReport).c_str());

        if (m_FileReadingFuture.valid() &&
            m_FileReadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            ImGui::CloseCurrentPopup();
            m_LoadingFilePath = "";
            m_LoadingProgressReport = "";
            const SResultReadScene& ResultScene = m_FileReadingFuture.get();
            if (ResultScene.Succeed)
            {
                // TODO: load scene for renderer
                _ASSERTE(m_ReadSceneCallback);
                m_ReadSceneCallback(ResultScene.pScene);
                m_pCurScene = ResultScene.pScene;
            }
            else
                showAlert(ResultScene.Message);
        }

        ImGui::EndPopup();
    }

    // 警告框
    m_GUIAlert.draw();

    // 缺少文件操作框
    m_RequestPopupModal.draw();

    ImGui::Begin(u8"设置", NULL, ImGuiWindowFlags_MenuBar);
    // 菜单栏

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu(u8"文件"))
        {
            if (ImGui::MenuItem(u8"打开"))
            {
                auto Result = NativeSystem::createOpenFileDialog("bsp;rmf;map;obj;mdl;*");
                if (Result)
                {
                    auto Path = Result.FilePath;
                    if (!m_ReadSceneCallback)
                    {
                        showAlert(u8"无法载入场景，未给GUI指定载入回调函数");
                    }
                    else if (!Path.empty())
                    {
                        m_LoadingFilePath = Path;
                        m_FileReadingFuture = std::async(readScene, m_LoadingFilePath);
                    }
                }
            }
            if (m_pCurScene && ImGui::MenuItem(u8"保存"))
            {
                auto Result = NativeSystem::createSaveFileDialog("obj");
                if (Result)
                {
                    CSceneObjWriter Writer;
                    Writer.addScene(m_pCurScene);
                    Writer.writeToFile(Result.FilePath);
                }
            }
            /*if (ImGui::MenuItem(u8"退出"))
            {
                glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
            }*/
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(u8"组件"))
        {
            ImGui::MenuItem(u8"FGD", nullptr, &m_Control.ShowWidgetFGD);
            ImGui::MenuItem(u8"帧率", nullptr, &m_Control.ShowWidgetFrameRate);
            ImGui::MenuItem(u8"日志", nullptr, &m_Control.ShowWidgetLog);
            
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
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
            u8"简易",
            u8"金源渲染"
        };

        static ERenderMethod LastMethod = m_RenderMethod;
        int RenderMethodIndex = static_cast<int>(std::find(RenderMethods.begin(), RenderMethods.end(), m_RenderMethod) - RenderMethods.begin());
        ImGui::Combo(u8"渲染器", &RenderMethodIndex, RenderMethodNames.data(), static_cast<int>(RenderMethods.size()));
        m_RenderMethod = RenderMethods[RenderMethodIndex];
        if (LastMethod != m_RenderMethod)
        {
            if (m_ChangeRenderMethodCallback)
            {
                m_ChangeRenderMethodCallback(m_RenderMethod);
                LastMethod = m_RenderMethod;
            }
            else
            {
                showAlert(u8"无法切换渲染器，未给GUI指定切换回调函数");
                m_RenderMethod = LastMethod;
            }
        }

        ImGui::Indent(20.0f);
        ImGui::Unindent();
    }

    if (m_RenderSettingCallback) m_RenderSettingCallback();

    if (ImGui::CollapsingHeader(u8"其他", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool IgnoreAllAlert = m_GUIAlert.getIgnoreAll();
        ImGui::Checkbox(u8"屏蔽所有警告", &IgnoreAllAlert);
        m_GUIAlert.setIgnoreAll(IgnoreAllAlert);
    }

    ImGui::End();

    // FGD设置
    if (m_Control.ShowWidgetFGD) m_FGD.draw();

    // 帧率
    if (m_Control.ShowWidgetFrameRate) m_GUIFrameRate.draw();

    // 日志
    if (m_Control.ShowWidgetLog) m_GUILog.draw();

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
}