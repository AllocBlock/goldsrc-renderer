#pragma once
#include "DrawableUI.h"
#include "Interactor.h"
#include "GuiFrameRate.h"
#include "GuiUtil.h"
#include "GuiFGD.h"
#include "GuiScene.h"
#include "SceneInfo.h"

#include <future>

struct SResultReadScene
{
    bool Succeed = false;
    std::string Message;
};

// TODO: custom event loop!
using ReadSceneCallbackFunc_t = std::function<void()>;

class CGUIMain : public IDrawableUI
{
public:
    CGUIMain();

    void showAlert(std::string vText);
    void log(std::string vText);

    void setSceneInfo(ptr<SSceneInfo> vSceneInfo)
    {
        m_pSceneInfo = vSceneInfo;
    }

    void setReadSceneCallback(ReadSceneCallbackFunc_t vCallback)
    {
        m_ReadSceneCallback = vCallback;
    }

    void setRenderSettingCallback(std::function<void()> vCallback)
    {
        m_RenderSettingCallback = vCallback;
    }

    void setSceneFocusedActor(CActor::Ptr vActor)
    {
        m_GuiScene.setFocusedActor(vActor);
    }

    void clearSceneFocusedActor()
    {
        m_GuiScene.clearFocusedActor();
    }

    static SResultReadScene readScene(std::filesystem::path vFilePath, ptr<SSceneInfo> voSceneInfo);
protected:
    virtual void _renderUIV() override;

private:
    SGuiUtil m_GuiUtil;
    CGuiFrameRate m_GuiFrameRate = CGuiFrameRate();
    CGuiFGD m_GuiFGD;
    CGuiScene m_GuiScene;

    ptr<SSceneInfo> m_pSceneInfo = nullptr;

    struct SControl
    {
        bool ShowWidgetFGD = false;
        bool ShowWidgetLog = false;
        bool ShowWidgetFrameRate = false;
        bool ShowWidgetScene = false;
    } m_Control;

    Common::SLoadingInfo m_LoadingInfo;
    std::future<SResultReadScene> m_LoadingFuture;

    std::function<void()> m_RenderSettingCallback = nullptr;
    ReadSceneCallbackFunc_t m_ReadSceneCallback = nullptr;
};