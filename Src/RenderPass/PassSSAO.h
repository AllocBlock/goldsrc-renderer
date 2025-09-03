#pragma once
#include "PassFullScreen.h"
#include "PipelineSSAO.h"

class CRenderPassSSAO : public CRenderPassFullScreen
{
public:
    inline static const std::string Name = "SSAO";

protected:
    virtual sptr<CPortSet> _createPortSetV() override
    {
        SPortDescriptor PortDesc;
        PortDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
        PortDesc.addInput("Depth", SPortInfo::createAnyOfUsage(EImageUsage::READ));
        return make<CPortSet>(PortDesc);
    }

    virtual void _initV() override
    {
        CRenderPassFullScreen::_initV();

        m_RenderInfoDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
        
        m_Pipeline.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);

        auto pDepthPort = m_pPortSet->getInputPort("Depth");
        m_Pipeline.setDepthImage(*pDepthPort->getImageV());
    }

    void _destroyV() override
    {
        m_Pipeline.destroy();
        CRenderPassFullScreen::_destroyV();
    }

    virtual void _updateV() override
    {
        m_Pipeline.updateUniformBuffer(m_pSceneInfo->pScene->getMainCamera());
    }

    virtual void _renderUIV() override
    {
        m_Pipeline.renderUI();
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() override
    {
        sptr<CCommandBuffer> pCommandBuffer = _getCommandBuffer();

        _beginCommand(pCommandBuffer);
        _beginRendering(pCommandBuffer, m_RenderInfoDescriptor.generateRendererInfo(m_ScreenExtent));
        m_Pipeline.bind(pCommandBuffer);
        _drawFullScreen(pCommandBuffer);
        _endRendering();
        _endCommand();
        return { pCommandBuffer->get() };
    }

private:
    CPipelineSSAO m_Pipeline;
    CRenderInfoDescriptor m_RenderInfoDescriptor;
};
