#include "GuiRenderer.h"

void CGuiRenderer::setTarget(ptr<CSceneRenderPass> vRenderer)
{
    _setTargetV(vRenderer);
}

void CGuiRenderer::draw()
{
    _drawV();
}