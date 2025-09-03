#pragma once
#include "Application.h"
#include "Interactor.h"
#include "PassGUI.h"
#include "PassShade.h"
#include "PassShadowMap.h"

class CApplicationShadowMap : public IApplication
{
public:
    CApplicationShadowMap() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __linkPasses();
    void __generateScene();

    sptr<CRenderPassGUI> m_pPassGUI = nullptr;
    sptr<CRenderPassShade> m_pPassShade = nullptr;
    sptr<CRenderPassShadowMap> m_pRenderPassShadowMap = nullptr;
    sptr<CInteractor> m_pInteractor = nullptr;

    sptr<CScene> m_pScene = nullptr;
};

