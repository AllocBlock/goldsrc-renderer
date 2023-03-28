#pragma once
#include "Application.h"
#include "Interactor.h"
#include "PipelineEnvironment.h"
#include "PassGUI.h"
#include "PassPBR.h"
#include "PassFullScreen.h"

class CApplicationPBR : public IApplication
{
public:
    CApplicationPBR() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    CCamera::Ptr m_pCamera = nullptr;
    ptr<CRenderPassGUI> m_pGUI = nullptr;
    ptr<CRenderPassPBR> m_pRenderPassPBR = nullptr;
    ptr<CRenderPassFullScreen> m_pRenderPassFullScreen = nullptr;
    ptr<CPipelineEnvironment> m_pPipelineEnv = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};

