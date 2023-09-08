#pragma once
#include "PassFullScreen.h"
#include "PipelineBlit.h"

// blit (copy) image from input to swapchain
class CRenderPassPresent : public CRenderPassFullScreenGeneral
{
public:
    inline static const std::string Name = "Present";

    void setSwapchainPort(CSourcePort::Ptr vSwapchainPort)
    {
        m_pSwapchainPort = vSwapchainPort;
    }

protected:
    virtual void _initV() override
    {
        _ASSERTE(m_pSwapchainPort);
        CRenderPassFullScreenGeneral::_initV();
        
        m_BlitPipeline.create(m_pDevice, get(), m_ScreenExtent, m_ImageNum);

        auto pInputPort = m_pPortSet->getInputPort("Main");
        for (size_t i = 0; i < m_pSwapchainPort->getImageNumV(); ++i)
        {
            m_BlitPipeline.setInputImage(pInputPort->getImageV(i), i);
        }
    }

    virtual void _initPortDescV(SPortDescriptor& vioDesc) override
    {
        vioDesc.addInput("Main", SPortFormat::createAnyOfUsage(EUsage::READ));
    }

    void _destroyV() override
    {
        m_BlitPipeline.destroy();
        CRenderPassFullScreenGeneral::_destroyV();
    }

    virtual CRenderPassDescriptor _getRenderPassDescV() override
    {
        return CRenderPassDescriptor::generateSingleSubpassDesc(m_pSwapchainPort);
    }

    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override
    {
        CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

        _beginWithFramebuffer(vImageIndex);
        m_BlitPipeline.bind(pCommandBuffer, vImageIndex);
        _drawFullScreen(pCommandBuffer);
        _endWithFramebuffer();
        return { pCommandBuffer->get() };
    }

    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        _ASSERTE(m_pSwapchainPort);
        return
        {
            m_pSwapchainPort->getImageV(vIndex)
        };
    }

    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        //return DefaultClearValueColor;
        std::vector<VkClearValue> Value(1);
        Value[0].color = { 1.0, 0.0, 0.0, 1.0 };
        return Value;
    }

private:
    CPipelineBlit m_BlitPipeline;

    CSourcePort::Ptr m_pSwapchainPort = nullptr;
};
