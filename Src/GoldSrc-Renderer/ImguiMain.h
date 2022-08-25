#pragma once
#include "IGUI.h"
#include "Interactor.h"
#include "ImguiAlert.h"
#include "ImguiFrameRate.h"
#include "ImguiLog.h"
#include "ImguiRenderer.h"
#include "ImguiFGD.h"
#include "ImguiRequestPopupModal.h"
#include "Scene.h"

#include <future>

enum class ERenderMethod
{
    DEFAULT,
    BSP
};

struct SResultReadScene
{
    bool Succeed = false;
    std::string Message;
    ptr<SScene> pScene;
};

// TODO: custom event loop!
using ReadSceneCallbackFunc_T = std::function<void(ptr<SScene>)>;
using ChangeRenderMethodCallbackFunc_T = std::function<void(ERenderMethod)>;

class CGUIMain : public IGUI
{
public:
    CGUIMain();

    void showAlert(std::string vText);
    void log(std::string vText);

    void setReadSceneCallback(ReadSceneCallbackFunc_T vCallback)
    {
        m_ReadSceneCallback = vCallback;
    }

    void setChangeRendererCallback(ChangeRenderMethodCallbackFunc_T vCallback)
    {
        m_ChangeRenderMethodCallback = vCallback;
    }

    void setRenderSettingCallback(std::function<void()> vCallback)
    {
        m_RenderSettingCallback = vCallback;
    }

    void setInteractor(ptr<CInteractor> vInteractor)
    {
        m_pInteractor = vInteractor;
    }

    static SResultReadScene readScene(std::filesystem::path vFilePath);
protected:
    virtual void _renderUIV() override;

private:
    ptr<CInteractor> m_pInteractor = nullptr;

    CImguiAlert m_GUIAlert = CImguiAlert();
    CImguiFrameRate m_GUIFrameRate = CImguiFrameRate();
    CImguiLog m_GUILog = CImguiLog();
    CImguiFGD m_FGD;
    CImguiRequestPopupModal m_RequestPopupModal;

    ptr<SScene> m_pCurScene = nullptr;

    struct SControl
    {
        bool ShowWidgetFGD = false;
        bool ShowWidgetLog = false;
        bool ShowWidgetFrameRate = false;
    } m_Control;

    ERenderMethod m_RenderMethod = ERenderMethod::BSP;
    std::filesystem::path m_LoadingFilePath = "";
    std::string m_LoadingProgressReport = "";
    std::future<SResultReadScene> m_FileReadingFuture;

    std::function<void()> m_RenderSettingCallback = nullptr;
    ReadSceneCallbackFunc_T m_ReadSceneCallback = nullptr;
    ChangeRenderMethodCallbackFunc_T m_ChangeRenderMethodCallback = nullptr;
};