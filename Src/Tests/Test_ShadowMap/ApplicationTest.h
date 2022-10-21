#pragma once
#include "Application.h"
#include "Interactor.h"
#include "PassGUI.h"
#include "PassShade.h"
#include "PassShadowMap.h"

class CApplicationTest : public IApplication
{
public:
    CApplicationTest() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    void __linkPasses();
    void __generateScene();

    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassShade> m_pPassShade = nullptr;
    ptr<CRenderPassShadowMap> m_pRenderPassShadowMap = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;

    CTempScene::Ptr m_pScene = nullptr;
};

