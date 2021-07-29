#include "ImguiRenderer.h"

void CImguiRenderer::setTarget(std::shared_ptr<CRendererScene> vRenderer)
{
    _setTargetV(vRenderer);
}

void CImguiRenderer::draw()
{
    _drawV();
}