#pragma once
#include "Application.h"
#include "GuiMain.h"
#include "Interactor.h"
#include "SceneInfoGoldSrc.h"
#include "PassScene.h"
#include "PassGUI.h"
#include "PassOutlineMask.h"
#include "PassOutlineEdge.h"

class CApplicationGoldSrc : public IApplication
{
public:
    CApplicationGoldSrc() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __recreateRenderer(ERenderMethod vMethod);
    void __linkPasses();

    ptr<CRenderPassScene> m_pPassScene = nullptr;
    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassOutlineMask> m_pPassOutlineMask = nullptr;
    ptr<CRenderPassOutlineEdge> m_pPassOutlineEdge = nullptr;
    ptr<CGUIMain> m_pMainUI = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
    CCamera::Ptr m_pCamera = nullptr;

    ptr<SSceneInfoGoldSrc> m_pSceneInfo = nullptr;
};
