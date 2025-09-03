#pragma once
#include "Application.h"
#include "GuiMain.h"
#include "Interactor.h"
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
    virtual std::vector<VkCommandBuffer> _getCommandBuffers() override;
    virtual void _destroyV() override;
    virtual void _onSwapchainRecreateV() override;

private:
    sptr<CGUIMain> m_pMainUI = nullptr;
    sptr<CInteractor> m_pInteractor = nullptr;

    const sptr<CRenderPassGraphInstance> m_pGraphInstance = make<CRenderPassGraphInstance>();
    sptr<SRenderPassGraph> m_pRenderPassGraph = nullptr;
    sptr<CRenderPassGraphUI> m_pRenderPassGraphUI = nullptr;
    bool m_NeedRecreateGraphInstance = true;
};
