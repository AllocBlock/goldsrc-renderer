#pragma once
#include "Application.h"
#include "Interactor.h"
#include "PassGUI.h"
#include "PassTest.h"

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

    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassTest> m_pPassMain = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};

