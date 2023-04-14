#include "GuiMain.h"
#include "SceneInterface.h"
#include "SceneCommon.h"
#include "InterfaceUI.h"
#include "NativeSystem.h"
#include "SceneObjWriter.h"
#include "Log.h"

#include <future>

#include "Environment.h"

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

    static auto RequestFilePathFunc = [=](std::filesystem::path vOriginPath, const std::filesystem::path& vAdditionalSearchDir, std::string vMessage, std::string vFilter) -> Scene::SRequestResultFilePath
    {
        size_t RetryTimes = 0;
        while (true)
        {
            std::filesystem::path FilePath;
            if (Environment::findFile(vOriginPath, vAdditionalSearchDir, true, FilePath))
            {
                return { true, FilePath };
            }
            else
            {
                std::string RetryString = (RetryTimes > 0 ? (u8"重试次数（" + std::to_string(RetryTimes) + u8"）") : "");
                auto Future = m_RequestPopupModal.show(u8"未找到文件", vMessage + "\n" + RetryString);
                ERequestAction ActionResult = Future.get();
                
                if (ActionResult == ERequestAction::MANUAL_FIND)
                {
                    auto SelectResult = Gui::createOpenFileDialog(vFilter);
                    if (SelectResult)
                        return { true, SelectResult.FilePath };
                    else
                        return { false, "" };
                }
                else if (ActionResult == ERequestAction::RETRY)
                {
                    ++RetryTimes;
                    continue;
                }
                else if (ActionResult == ERequestAction::IGNORE_ || ActionResult == ERequestAction::CANCEL)
                    return { false, "" };
                else
                    _SHOULD_NOT_GO_HERE;
            }
        }
    };

    Scene::setGlobalRequestFilePathFunc(RequestFilePathFunc);
}

SResultReadScene CGUIMain::readScene(std::filesystem::path vFilePath, ptr<SSceneInfo> voSceneInfo)
{
    _ASSERTE(voSceneInfo);
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
        SceneInterface::read(Extension, vFilePath, voSceneInfo);
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
    Log::log("警告: " + vText);
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
                m_ReadSceneCallback();
                
                m_GUIScene.setScene(m_pSceneInfo->pScene);
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
                        _ASSERTE(m_pSceneInfo);
                        m_LoadingFilePath = Path;
                        m_FileReadingFuture = std::async(readScene, m_LoadingFilePath, m_pSceneInfo);
                    }
                }
            }
            if (m_pSceneInfo && UI::menuItem(u8"保存"))
            {
                auto Result = Gui::createSaveFileDialog("obj");
                if (Result)
                {
                    CSceneObjWriter Writer;
                    Writer.addSceneInfo(m_pSceneInfo);
                    Writer.writeToFile(Result.FilePath);
                }
            }
            UI::endMenu();
        }
        if (UI::beginMenu(u8"组件"))
        {
            UI::menuItem(u8"FGD", &m_Control.ShowWidgetFGD);
            UI::menuItem(u8"帧率", &m_Control.ShowWidgetFrameRate);
            UI::menuItem(u8"日志", &m_Control.ShowWidgetLog);
            UI::menuItem(u8"场景", &m_Control.ShowWidgetScene);
            
            UI::endMenu();
        }
        UI::endMenuBar();
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

    // 场景
    if (m_Control.ShowWidgetScene) m_GUIScene.draw();

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