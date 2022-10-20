#pragma once
#include "Application.h"
#include "GUIPass.h"
#include "RendererTest.h"
#include "Interactor.h"

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

    ptr<CGUIRenderPass> m_pPassGUI = nullptr;
    ptr<CRenderPassTest> m_pPassMain = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};

