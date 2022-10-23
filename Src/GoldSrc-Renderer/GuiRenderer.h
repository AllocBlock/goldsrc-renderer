#pragma once
#include "PassGoldSrc.h"
#include "PassScene.h"

class CGuiRenderer
{
public:
    void setTarget(ptr<CRenderPassScene> vRenderer);
    void draw();
protected:
    virtual void _setTargetV(ptr<CRenderPassScene> vRenderer) = 0;
    virtual void _drawV() = 0;
};
