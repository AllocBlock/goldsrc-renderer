#pragma once
#include "VulkanRenderer.h"
#include "RendererScene.h"

#include <vector>
#include <string>
#include <istream>

class CImguiRenderer
{
public:
    void setTarget(ptr<CRendererScene> vRenderer);
    void draw();
protected:
    virtual void _setTargetV(ptr<CRendererScene> vRenderer) = 0;
    virtual void _drawV() = 0;
};

