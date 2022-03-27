#pragma once
#include "ImguiBase.h"
#include "SceneInteractor.h"
#include "ImguiAlert.h"
#include "ImguiFrameRate.h"
#include "ImguiLog.h"
#include "ImguiSelectFile.h"
#include "ImguiRenderer.h"
#include "ImguiFGD.h"
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
    std::shared_ptr<SScene> pScene;
};

// TODO: custom event loop!
using ReadSceneCallbackFunc_T = std::function<void(std::shared_ptr<SScene>)>;
using ChangeRenderMethodCallbackFunc_T = std::function<void(ERenderMethod)>;

class CGUIMain : public CImguiBase
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

    void setInteractor(std::shared_ptr<CSceneInteractor> vInteractor)
    {
        m_pInteractor = vInteractor;
    }

    static SResultReadScene readScene(std::filesystem::path vFilePath);
protected:
    virtual void _renderUIV() override;

private:
    std::shared_ptr<CSceneInteractor> m_pInteractor = nullptr;

    CImguiAlert m_GUIAlert = CImguiAlert();
    CImguiFrameRate m_GUIFrameRate = CImguiFrameRate();
    CImguiLog m_GUILog = CImguiLog();
    CImguiSelectFile m_FileSelection;
    CImguiFGD m_FGD;

    ERenderMethod m_RenderMethod = ERenderMethod::BSP;
    std::filesystem::path m_LoadingFilePath = "";
    std::string m_LoadingProgressReport = "";
    std::future<SResultReadScene> m_FileReadingFuture;

    ReadSceneCallbackFunc_T m_ReadSceneCallback = nullptr;
    ChangeRenderMethodCallbackFunc_T m_ChangeRenderMethodCallback = nullptr;
};