#pragma once
#include "PassFullScreen.h"
#include "PipelineSSAO.h"

class CRenderPassSSAO : public CRenderPassFullScreen
{
public:
    inline static const std::string Name = "SSAO";

protected:
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override
    {
        vioDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
        vioDesc.addInput("Depth", SPortInfo::createAnyOfUsage(EImageUsage::READ));
    }

    virtual void _initV() override
    {
        CRenderPassFullScreen::_initV();
        
        m_Pipeline.create(m_pDevice, get(), m_ScreenExtent, m_ImageNum);

        auto pDepthPort = m_pPortSet->getInputPort("Depth");
        for (size_t i = 0; i < m_ImageNum; ++i)
        {
            m_Pipeline.setDepthImage(pDepthPort->getImageV(i), i);
        }
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

    virtual void _updateV(uint32_t vImageIndex) override
    {
        m_Pipeline.updateUniformBuffer(vImageIndex, m_pSceneInfo->pScene->getMainCamera());
    }

    virtual void _renderUIV() override
    {
        m_Pipeline.renderUI();
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override
    {
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

        _beginWithFramebuffer(vImageIndex);
        m_Pipeline.bind(pCommandBuffer, vImageIndex);
        _drawFullScreen(pCommandBuffer);
        _endWithFramebuffer();
        return { pCommandBuffer->get() };
    }

    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex)
        };
    }

    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColor;
    }

private:
    CPipelineSSAO m_Pipeline;
};
