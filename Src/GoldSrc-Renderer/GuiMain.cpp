#include "GuiMain.h"
#include "SceneInterface.h"
#include "SceneCommon.h"
#include "InterfaceUI.h"
#include "NativeSystem.h"
#include "SceneObjWriter.h"
#include "Log.h"

#include <future>

CGUIMain::CGUIMain()
{
    Log::setLogObserverFunc([=](std::string vText)
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
            auto SelectResult = Gui::createOpenFileDialog(vFilter);
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
    Log::log(u8"警告: " + vText);
}

void CGUIMain::log(std::string vText)
{
    m_GUILog.log(vText);
}

void CGUIMain::_renderUIV()
{
    glm::vec2 Center = UI::getDisplayCenter();

    // 文件加载信息框
    if (!m_RequestPopupModal.isShow() && !m_LoadingFilePath.empty() && !UI::isPopupOpen(u8"提示"))
        UI::openPopup(u8"提示"); // 打开加载提示
    UI::setNextWindowPos(Center, UI::ESetVariableCondition::APPEARING, glm::vec2(0.5f, 0.5f));
    if (UI::beginPopupModal(u8"提示", nullptr, UI::EWindowFlag::ALWAYS_AUTO_RESIZE))
    {
        UI::text(u8"加载文件中...");
        UI::text(u8"[ " + m_LoadingFilePath.u8string() + u8" ]");
        if (!m_LoadingProgressReport.empty()) UI::text(u8"进度：" + m_LoadingProgressReport);

        if (m_FileReadingFuture.valid() &&
            m_FileReadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            UI::closeCurrentPopup();
            m_LoadingFilePath = "";
            m_LoadingProgressReport = "";
            const SResultReadScene& ResultScene = m_FileReadingFuture.get();
            if (ResultScene.Succeed)
            {
                _ASSERTE(m_ReadSceneCallback);
                m_ReadSceneCallback(ResultScene.pScene);
                m_pCurScene = ResultScene.pScene;
            }
            else
                showAlert(ResultScene.Message);
        }

        UI::endPopup();
    }

    // 警告框
    m_GUIAlert.draw();

    // 缺少文件操作框
    m_RequestPopupModal.draw();

    UI::beginWindow(u8"设置", NULL, UI::EWindowFlag::MENU_BAR);
    // 菜单栏

    if (UI::beginMenuBar())
    {
        if (UI::beginMenu(u8"文件"))
        {
            if (UI::menuItem(u8"打开"))
            {
                auto Result = Gui::createOpenFileDialog("bsp;rmf;map;obj;mdl;*");
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
            if (m_pCurScene && UI::menuItem(u8"保存"))
            {
                auto Result = Gui::createSaveFileDialog("obj");
                if (Result)
                {
                    CSceneObjWriter Writer;
                    Writer.addScene(m_pCurScene);
                    Writer.writeToFile(Result.FilePath);
                }
            }
            /*if (UI::menuItem(u8"退出"))
            {
                glfwSetWindowShouldClose(m_pWindow, GLFW_TRUE);
            }*/
            UI::endMenu();
        }
        if (UI::beginMenu(u8"组件"))
        {
            UI::menuItem(u8"FGD", &m_Control.ShowWidgetFGD);
            UI::menuItem(u8"帧率", &m_Control.ShowWidgetFrameRate);
            UI::menuItem(u8"日志", &m_Control.ShowWidgetLog);
            
            UI::endMenu();
        }
        UI::endMenuBar();
    }

    // 渲染设置
    if (UI::collapse(u8"渲染", true))
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
        UI::combo(u8"渲染器", RenderMethodNames, RenderMethodIndex);
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
    }

    if (m_RenderSettingCallback) m_RenderSettingCallback();

    if (UI::collapse(u8"其他", true))
    {
        bool IgnoreAllAlert = m_GUIAlert.getIgnoreAll();
        UI::toggle(u8"屏蔽所有警告", IgnoreAllAlert);
        m_GUIAlert.setIgnoreAll(IgnoreAllAlert);
    }

    UI::endWindow();

    // FGD设置
    if (m_Control.ShowWidgetFGD) m_FGD.draw();

    // 帧率
    if (m_Control.ShowWidgetFrameRate) m_GUIFrameRate.draw();

    // 日志
    if (m_Control.ShowWidgetLog) m_GUILog.draw();

    // DEBUG
    if (UI::button("test alert"))
    {
        showAlert("alert1");
        showAlert("alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2alert2");
        showAlert("alert3");
    }
    if (UI::button("test log"))
    {
        log("log1");
        log("log2log2log2log2log2log2log2log2log2log2log2log2log2log2log2log2log2");
        log("log3");
    }
}