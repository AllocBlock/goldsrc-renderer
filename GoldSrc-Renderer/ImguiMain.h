#pragma once
#include "GUIBase.h"
#include "Interactor.h"
#include "VulkanRenderer.h"
#include "ImguiAlert.h"
#include "ImguiFrameRate.h"
#include "ImguiLog.h"

#include <future>

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

    void setRenderer(std::shared_ptr<CVulkanRenderer> vRenderer) { m_pRenderer = vRenderer; }
    std::shared_ptr<CVulkanRenderer> getRenderer() { return m_pRenderer; }
    void setInteractor(std::shared_ptr<CInteractor> vInteractor) { m_pInteractor = vInteractor; }
    std::shared_ptr<CInteractor> getInteractor() { return m_pInteractor; }
protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;

private:
    void __drawGUI();

    std::shared_ptr<CVulkanRenderer> m_pRenderer = nullptr;
    std::shared_ptr<CInteractor> m_pInteractor = nullptr;

    CImguiAlert m_GUIAlert = CImguiAlert();
    CImguiFrameRate m_GUIFrameRate = CImguiFrameRate();
    CImguiLog m_GUILog = CImguiLog();

    std::filesystem::path m_LoadingFilePath = "";
    std::string m_LoadingProgressReport = "";
    std::future<SResultReadScene> m_FileReadingPromise;
};