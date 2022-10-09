#pragma once
#include "IApplication.h"
#include "GUIPass.h"
#include "PassShade.h"
#include "Interactor.h"
#include "TempScene.h"

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
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    void __linkPasses();

    ptr<CCamera> m_pCamera = nullptr;

    ptr<CGUIRenderPass> m_pPassGUI = nullptr;
    ptr<CRenderPassShade> m_pPassShade = nullptr;

    ptr<CInteractor> m_pInteractor = nullptr;

    CTempScene::Ptr m_pScene = nullptr;
};
