#pragma once
#include "ApplicationBase.h"
#include "GUI.h"
#include "GUIRenderer.h"
#include "RendererPBR.h"
#include "Interactor.h"

class CApplicationTest : public CApplicationBase
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
    ptr<CGUIRenderer> m_pGUI = nullptr;
    ptr<CRendererPBR> m_pRenderer = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};

