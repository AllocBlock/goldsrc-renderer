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
    virtual void _destroyV() override;

private:
    void __linkPasses();

    sptr<CCamera> m_pCamera = nullptr;
    sptr<CRenderPassGUI> m_pGUI = nullptr;
    sptr<CRenderPassPBR> m_pRenderPassPBR = nullptr;
    sptr<CRenderPassFullScreen> m_pRenderPassFullScreen = nullptr;
    sptr<CPipelineEnvironment> m_pPipelineEnv = nullptr;
    sptr<CInteractor> m_pInteractor = nullptr;
};

