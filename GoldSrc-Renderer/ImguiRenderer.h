#pragma once
#include "VulkanRenderer.h"
#include "RendererScene.h"

#include <vector>
#include <string>
#include <istream>

class CImguiRenderer
{
public:
    void setTarget(std::shared_ptr<CRendererScene> vRenderer);
    void draw();
protected:
    virtual void _setTargetV(std::shared_ptr<CRendererScene> vRenderer) = 0;
    virtual void _drawV() = 0;
};

