#include "GuiRenderer.h"

void CImguiRenderer::setTarget(ptr<CSceneRenderPass> vRenderer)
{
    _setTargetV(vRenderer);
}

void CImguiRenderer::draw()
{
    _drawV();
}