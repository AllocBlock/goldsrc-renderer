#pragma once
#include "DrawableUI.h"
#include "Interactor.h"
#include "GuiAlert.h"
#include "GuiFrameRate.h"
#include "GuiLog.h"
#include "GuiFGD.h"
#include "GuiScene.h"
#include "GuiRequestPopupModal.h"
#include "SceneInfoGoldSrc.h"

#include <future>

struct SResultReadScene
{
    bool Succeed = false;
    std::string Message;
    ptr<SSceneInfoGoldSrc> pSceneInfo;
};

// TODO: custom event loop!
using ReadSceneCallbackFunc_T = std::function<void(ptr<SSceneInfoGoldSrc>)>;

class CGUIMain : public IDrawableUI
{
public:
    CGUIMain();

    void showAlert(std::string vText);
    void log(std::string vText);

    void setReadSceneCallback(ReadSceneCallbackFunc_T vCallback)
    {
        m_ReadSceneCallback = vCallback;
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

    CGuiAlert m_GUIAlert = CGuiAlert();
    CGuiFrameRate m_GUIFrameRate = CGuiFrameRate();
    CGuiLog m_GUILog = CGuiLog();
    CGuiFGD m_FGD;
    CGuiScene m_GUIScene;
    CGuiRequestPopupModal m_RequestPopupModal;

    ptr<SSceneInfoGoldSrc> m_pCurSceneInfo = nullptr;

    struct SControl
    {
        bool ShowWidgetFGD = false;
        bool ShowWidgetLog = false;
        bool ShowWidgetFrameRate = false;
        bool ShowWidgetScene = false;
    } m_Control;
    
    std::filesystem::path m_LoadingFilePath = "";
    std::string m_LoadingProgressReport = "";
    std::future<SResultReadScene> m_FileReadingFuture;

    std::function<void()> m_RenderSettingCallback = nullptr;
    ReadSceneCallbackFunc_T m_ReadSceneCallback = nullptr;
};