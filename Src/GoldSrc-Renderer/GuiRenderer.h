#pragma once
#include "PassGoldSrc.h"
#include "ScenePass.h"

#include <vector>
#include <string>
#include <istream>

class CGuiRenderer
{
public:
    void setTarget(ptr<CSceneRenderPass> vRenderer);
    void draw();
protected:
    virtual void _setTargetV(ptr<CSceneRenderPass> vRenderer) = 0;
    virtual void _drawV() = 0;
};

