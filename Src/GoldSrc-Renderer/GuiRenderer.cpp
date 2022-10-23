#include "GuiRenderer.h"

void CGuiRenderer::setTarget(ptr<CRenderPassScene> vRenderer)
{
    _setTargetV(vRenderer);
}

void CGuiRenderer::draw()
{
    _drawV();
}