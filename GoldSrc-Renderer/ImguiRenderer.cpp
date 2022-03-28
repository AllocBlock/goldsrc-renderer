#include "ImguiRenderer.h"

void CImguiRenderer::setTarget(ptr<CRendererScene> vRenderer)
{
    _setTargetV(vRenderer);
}

void CImguiRenderer::draw()
{
    _drawV();
}