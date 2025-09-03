#pragma once
#include "Application.h"
#include "Interactor.h"
#include "PassGUI.h"
#include "PassTest.h"

class CApplicationEnvMap : public IApplication
{
public:
    CApplicationEnvMap() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    sptr<CRenderPassGUI> m_pPassGUI = nullptr;
    sptr<CRenderPassSprite> m_pPassMain = nullptr;
    sptr<CInteractor> m_pInteractor = nullptr;
};
