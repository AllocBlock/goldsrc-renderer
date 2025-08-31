#pragma once
#include "PassFullScreen.h"
#include "PipelineSSAO.h"

class CRenderPassSSAO : public CRenderPassFullScreen
{
public:
    inline static const std::string Name = "SSAO";

protected:
    virtual CPortSet::Ptr _createPortSetV() override
    {
        SPortDescriptor PortDesc;
        PortDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
        PortDesc.addInput("Depth", SPortInfo::createAnyOfUsage(EImageUsage::READ));
        return make<CPortSet>(PortDesc);
    }

    virtual void _initV() override
    {
        CRenderPassFullScreen::_initV();
        
        m_Pipeline.create(m_pDevice, get(), m_ScreenExtent);

        auto pDepthPort = m_pPortSet->getInputPort("Depth");
        m_Pipeline.setDepthImage(pDepthPort->getImageV());
    }

    void _destroyV() override
    {
        m_Pipeline.destroy();
        CRenderPassFullScreen::_destroyV();
    }

    virtual CRenderPassDescriptor _getRenderPassDescV() override
    {
        return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
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
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer();

        _beginWithFramebuffer();
        m_Pipeline.bind(pCommandBuffer);
        _drawFullScreen(pCommandBuffer);
        _endWithFramebuffer();
        return { pCommandBuffer->get() };
    }

    virtual std::vector<VkImageView> _getAttachmentsV() override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV()
        };
    }

    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColor;
    }

private:
    CPipelineSSAO m_Pipeline;
};
