#pragma once
#include "GUIBase.h"
#include "Interactor.h"
#include "VulkanRenderer.h"
#include "ImguiAlert.h"
#include "ImguiFrameRate.h"
#include "ImguiLog.h"

struct SResultReadScene
{
    bool Succeed = false;
    std::string Message;
    std::shared_ptr<SScene> pScene;
};

class CGUIMain : public CGUIBase
{
public:
    CGUIMain() = delete;
    CGUIMain(GLFWwindow* vpWindow);

    void showAlert(std::string vText);
    void log(std::string vText);

    static SResultReadScene readScene(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);

    std::shared_ptr<CVulkanRenderer> getRenderer() { return m_pRenderer; }
protected:
    virtual void _initV() override;
    virtual void _renderV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

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