#pragma once
#include "ImguiRenderer.h"
#include "SimpleRenderer.h"

class CImguiRendererSimple : public CImguiRenderer
{
protected:
    virtual void _setTargetV(std::shared_ptr<CRendererScene> vRenderer) override;
    virtual void _drawV() override;

private:
    std::shared_ptr<CRendererSceneSimple> m_pRenderer = nullptr;
};

