#include "GuiMain.h"
#include "SceneInterface.h"
#include "SceneCommon.h"
#include "InterfaceGui.h"
#include "NativeSystem.h"
#include "SceneObjWriter.h"
#include "Log.h"

#include <future>

#include "Environment.h"

CGUIMain::CGUIMain()
{
    Log::setLogObserverFunc([=](std::string vText)
    {
        m_GuiUtil.Log.log(vText);
    });

    static std::function<void(std::string)> ProgressReportFunc = [=](std::string vMessage)
    {
        m_LoadingInfo.Message = vMessage;
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
                m_GuiUtil.Log.log(u8"未找到文件：" + vOriginPath.string() + " [" + vMessage + "]", ELogLevel::WARNING);
                return { false, "" };
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
    m_GuiUtil.Alert.add(vText);
    Log::log("警告: " + vText);
}

void CGUIMain::log(std::string vText)
{
    m_GuiUtil.Log.log(vText);
}

void CGUIMain::_renderUIV()
{
    // 更新文件加载信息
    if (m_LoadingFuture.valid())
    {
        m_GuiUtil.Loading.update(m_LoadingInfo);

        if (m_LoadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            m_GuiUtil.Loading.end();

            const SResultReadScene& ResultScene = m_LoadingFuture.get();
            if (ResultScene.Succeed)
            {
                _ASSERTE(m_ReadSceneCallback);
                m_ReadSceneCallback();

                m_GuiScene.setScene(m_pSceneInfo->pScene);
            }
            else
                showAlert(ResultScene.Message);
        }
    }
    m_GuiUtil.Log.setShow(m_Control.ShowWidgetLog);

    // util
    m_GuiUtil.renderUI();

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
                        m_LoadingInfo.FileName = Path;
                        m_LoadingFuture = std::async(readScene, Path, m_pSceneInfo);
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
        bool IgnoreAllAlert = m_GuiUtil.Alert.getIgnoreAll();
        UI::toggle(u8"屏蔽所有警告", IgnoreAllAlert);
        m_GuiUtil.Alert.setIgnoreAll(IgnoreAllAlert);
    }

    UI::endWindow();

    // FGD设置
    if (m_Control.ShowWidgetFGD) m_GuiFGD.draw();

    // 帧率
    if (m_Control.ShowWidgetFrameRate) m_GuiFrameRate.draw();

    // 场景
    if (m_Control.ShowWidgetScene) m_GuiScene.draw();

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
        m_GuiUtil.Log.log("this is a warning", ELogLevel::WARNING);
        m_GuiUtil.Log.log("this is an error", ELogLevel::ERROR);
    }
}