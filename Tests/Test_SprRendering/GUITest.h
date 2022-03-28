#pragma once
#include "GUIBase.h"
#include "Camera.h"
#include "RendererTest.h"

class CGUITest : public CGUIBase
{
public:
    CGUITest() = default;
    void setCamera(ptr<CCamera> vCamera) { m_pCamera = vCamera; }
    void setRenderer(ptr<CRendererTest> vRenderer) { m_pRenderer = vRenderer; }

protected:
    virtual void _updateV(uint32_t vImageIndex) override;

private:
    void __drawGUI();

    ptr<CCamera> m_pCamera = nullptr;
    ptr<CRendererTest> m_pRenderer = nullptr;
};

