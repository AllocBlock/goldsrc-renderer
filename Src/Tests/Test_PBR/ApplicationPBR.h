#pragma once
#include "Application.h"
#include "DrawableUI.h"
#include "PassGUI.h"
#include "RenderPassPBR.h"
#include "RenderPassFullScreen.h"
#include "PipelineEnvironment.h"
#include "Interactor.h"

class CApplicationPBR : public IApplication
{
public:
    CApplicationPBR() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    void __linkPasses();

    ptr<CCamera> m_pCamera = nullptr;
    ptr<CRenderPassGUI> m_pGUI = nullptr;
    ptr<CRenderPassPBR> m_pRenderPassPBR = nullptr;
    ptr<CRenderPassFullScreen> m_pRenderPassFullScreen = nullptr;
    ptr<CPipelineEnvironment> m_pPipelineEnv = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};

