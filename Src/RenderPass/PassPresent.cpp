#include "PassPresent.h"

void CRenderPassPresent::setSwapchainPort(CSourcePort::Ptr vSwapchainPort)
{
    m_pSwapchainPort = vSwapchainPort;
}

void CRenderPassPresent::_initV()
{
    _ASSERTE(m_pSwapchainPort);
    CRenderPassFullScreen::_initV();
        
    m_BlitPipeline.create(m_pDevice, get(), m_ScreenExtent, m_ImageNum);

    auto pInputPort = m_pPortSet->getInputPort("Main");
    for (size_t i = 0; i < m_pSwapchainPort->getActualNumV(); ++i)
    {
        m_BlitPipeline.setInputImage(pInputPort->getImageV(i), i);
    }
}

void CRenderPassPresent::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInput("Main", SPortInfo::createAnyOfUsage(EImageUsage::READ));
}

void CRenderPassPresent::_destroyV()
{
    m_BlitPipeline.destroy();
    CRenderPassFullScreen::_destroyV();
}

CRenderPassDescriptor CRenderPassPresent::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pSwapchainPort);
}

std::vector<VkCommandBuffer> CRenderPassPresent::_requestCommandBuffersV(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    _beginWithFramebuffer(vImageIndex);
    m_BlitPipeline.bind(pCommandBuffer, vImageIndex);
    _drawFullScreen(pCommandBuffer);
    _endWithFramebuffer();
    return { pCommandBuffer->get() };
}

std::vector<VkImageView> CRenderPassPresent::_getAttachmentsV(uint32_t vIndex)
{
    _ASSERTE(m_pSwapchainPort);
    return
    {
        m_pSwapchainPort->getImageV(vIndex)
    };
}

std::vector<VkClearValue> CRenderPassPresent::_getClearValuesV()
{
    return DefaultClearValueColor;
}
