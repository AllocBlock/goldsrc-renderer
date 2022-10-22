#pragma once
#include "Application.h"
#include "ScenePass.h"
#include "PassGUI.h"
#include "PassOutlineMask.h"
#include "PassOutlineEdge.h"
#include "GuiMain.h"
#include "Interactor.h"
#include "Scene.h"

class CApplicationGoldSrc : public IApplication
{
public:
    CApplicationGoldSrc() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createV() override;
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyV() override;
    virtual void _destroyV() override;

private:
    void __recreateRenderer(ERenderMethod vMethod = ERenderMethod::BSP);
    void __linkPasses();

    ptr<CSceneRenderPass> m_pPassScene = nullptr;
    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<COutlineMaskRenderPass> m_pPassOutlineMask = nullptr;
    ptr<COutlineEdgeRenderPass> m_pPassOutlineEdge = nullptr;
    ptr<CGUIMain> m_pMainUI = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
    CCamera::Ptr m_pCamera = nullptr;

    ptr<SScene> m_pScene = nullptr;
};

