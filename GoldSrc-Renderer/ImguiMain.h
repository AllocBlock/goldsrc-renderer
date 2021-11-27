#pragma once
#include "GUIBase.h"
#include "SceneInteractor.h"
#include "RendererScene.h"
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

class CGUIMain : public CGUIBase
{
public:
    CGUIMain();

    void showAlert(std::string vText);
    void log(std::string vText);

    static SResultReadScene readScene(std::filesystem::path vFilePath);

    void setInteractor(std::shared_ptr<CSceneInteractor> vInteractor) { m_pInteractor = vInteractor; }
    std::shared_ptr<CSceneInteractor> getInteractor() { return m_pInteractor; }
protected:
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _recreateV() override;
    virtual void _destroyV() override;

private:
    void __drawGUI();
    void __recreateRenderer();

    std::shared_ptr<CCamera> m_pCamera = nullptr;
    std::shared_ptr<SScene> m_pScene = nullptr;
    std::shared_ptr<CRendererScene> m_pRenderer = nullptr;
    std::shared_ptr<CSceneInteractor> m_pInteractor = nullptr;

    CImguiAlert m_GUIAlert = CImguiAlert();
    CImguiFrameRate m_GUIFrameRate = CImguiFrameRate();
    CImguiLog m_GUILog = CImguiLog();
    CImguiSelectFile m_FileSelection;
    CImguiFGD m_FGD;
    std::shared_ptr<CImguiRenderer> m_pGuiRenderer = nullptr;

    ERenderMethod m_RenderMethod = ERenderMethod::BSP;
    std::filesystem::path m_LoadingFilePath = "";
    std::string m_LoadingProgressReport = "";
    std::future<SResultReadScene> m_FileReadingFuture;
};