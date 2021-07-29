#pragma once
#include "ImguiRenderer.h"
#include "VulkanRenderer.h"

class CImguiRendererGoldSrc : public CImguiRenderer
{
protected:
    virtual void _setTargetV(std::shared_ptr<CRendererScene> vRenderer) override;
    virtual void _drawV() override;

private:
    std::shared_ptr<CRendererSceneGoldSrc> m_pRenderer = nullptr;
};

