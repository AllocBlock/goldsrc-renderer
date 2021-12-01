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
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    std::shared_ptr<CGUITest> m_pGUI = nullptr;
    std::shared_ptr<CRendererTest> m_pRenderer = nullptr;
    std::shared_ptr<CInteractor> m_pInteractor = nullptr;
};

