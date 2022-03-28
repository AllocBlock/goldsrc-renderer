#pragma once
#include "ApplicationBase.h"
#include "GUITest.h"
#include "RendererTest.h"
#include "Interactor.h"

class CApplicationTest : public CApplicationBase
{
public:
    CApplicationTest() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    ptr<CGUITest> m_pGUI = nullptr;
    ptr<CRendererTest> m_pRenderer = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};

