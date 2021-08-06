#pragma once
#include "GUIBase.h"
#include "Camera.h"
#include "RendererTest.h"

class CGUITest : public CGUIBase
{
public:
    CGUITest() = default;
    void setCamera(std::shared_ptr<CCamera> vCamera) { m_pCamera = vCamera; }
    void setRenderer(std::shared_ptr<CRendererTest> vRenderer) { m_pRenderer = vRenderer; }

protected:
    virtual void _updateV(uint32_t vImageIndex) override;

private:
    void __drawGUI();

    std::shared_ptr<CCamera> m_pCamera = nullptr;
    std::shared_ptr<CRendererTest> m_pRenderer = nullptr;
};
