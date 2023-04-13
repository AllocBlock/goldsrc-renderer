#pragma once
#include "Application.h"
#include "GuiMain.h"
#include "Interactor.h"
#include "SceneInfo.h"
#include "PassGoldSrc.h"
#include "PassGUI.h"
#include "PassOutlineMask.h"
#include "PassOutlineEdge.h"
#include "PassVisualize.h"
#include "RenderPassGraph.h"
#include "RenderPassGraphUI.h"

class CApplicationGoldSrc : public IApplication
{
public:
    CApplicationGoldSrc() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    ptr<CRenderPassGoldSrc> m_pPassGoldSrc = nullptr;
    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassOutlineMask> m_pPassOutlineMask = nullptr;
    ptr<CRenderPassOutlineEdge> m_pPassOutlineEdge = nullptr;
    ptr<CRenderPassVisualize> m_pPassVisualize = nullptr;

    ptr<CGUIMain> m_pMainUI = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
    ptr<SSceneInfo> m_pSceneInfo = make<SSceneInfo>();

    ptr<SRenderPassGraph> m_pRenderPassGraph = make<SRenderPassGraph>();
    ptr<CRenderPassGraphUI> m_pRenderPassGraphUI = nullptr;
};
