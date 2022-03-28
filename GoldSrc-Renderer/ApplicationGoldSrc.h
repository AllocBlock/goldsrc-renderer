#pragma once
#include "ApplicationBase.h"
#include "RendererScene.h"
#include "GUIRenderer.h"
#include "ImguiMain.h"
#include "Interactor.h"
#include "Scene.h"

class CApplicationGoldSrc : public CApplicationBase
{
public:
    CApplicationGoldSrc() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    void __recreateRenderer(ERenderMethod vMethod = ERenderMethod::BSP);

    std::shared_ptr<CRendererScene> m_pRenderer = nullptr;
    std::shared_ptr<CGUIRenderer> m_pGUI = nullptr;
    std::shared_ptr<CGUIMain> m_pMainUI = nullptr;
    std::shared_ptr<CInteractor> m_pInteractor = nullptr;
    std::shared_ptr<CCamera> m_pCamera = nullptr;

    std::shared_ptr<SScene> m_pScene = nullptr;
};

