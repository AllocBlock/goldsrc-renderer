#include "PassFullScreen.h"
#include "RenderPassDescriptor.h"

void CRenderPassFullScreen::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();

    __createVertexBuffer();

    VkExtent2D RefExtent = { 0, 0 };
    if (!_dumpInputPortExtent("Main", RefExtent)) return;
    __createPipeline(RefExtent);
}

void CRenderPassFullScreen::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
}

CRenderPassDescriptor CRenderPassFullScreen::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

bool CRenderPassFullScreen::_dumpReferenceExtentV(VkExtent2D& voExtent)
{
    return _dumpInputPortExtent("Main", voExtent);
}

std::vector<VkImageView> CRenderPassFullScreen::_getAttachmentsV(uint32_t vIndex)
{
    return
    {
        m_pPortSet->getOutputPort("Main")->getImageV(vIndex)
    };
}

std::vector<VkClearValue> CRenderPassFullScreen::_getClearValuesV()
{
    return DefaultClearValueColor;
}

std::vector<VkCommandBuffer> CRenderPassFullScreen::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline->isValid());

    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    _beginWithFramebuffer(vImageIndex);

    if (m_pVertexBuffer->isValid())
    {
        pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
        m_pPipeline->bind(pCommandBuffer, vImageIndex);
        pCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
    }
    
    _endWithFramebuffer();
    return { pCommandBuffer->get() };
}

void CRenderPassFullScreen::_destroyV()
{
    destroyAndClear(m_pPipeline);
    destroyAndClear(m_pVertexBuffer);

    CRenderPassSingleFrameBuffer::_destroyV();
}

void CRenderPassFullScreen::__createPipeline(VkExtent2D vExtent)
{
    _ASSERTE(isValid() && m_pPipeline);
    m_pPipeline->create(m_pDevice, get(), vExtent, m_ImageNum);

    for (const auto& Callback : m_PipelineCreateCallbackSet)
        Callback();
}

void CRenderPassFullScreen::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SFullScreenPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}

void CRenderPassFullScreen::__generateScene()
{
    m_PointDataSet =
    {
        SFullScreenPointData{glm::vec2(-1.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(-1.0f, 3.0f)},
        SFullScreenPointData{glm::vec2(3.0f, -1.0f)},
    };
}

void CRenderPassFullScreenGeneral::_initV()
{
    __createVertexBuffer();
}

void CRenderPassFullScreenGeneral::_destroyV()
{
    destroyAndClear(m_pVertexBuffer);
    IRenderPass::_destroyV();
}

void CRenderPassFullScreenGeneral::_bindVertexBuffer(CCommandBuffer::Ptr vCommandBuffer)
{
    _ASSERTE(m_pVertexBuffer->isValid());
    vCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
}

void CRenderPassFullScreenGeneral::_drawFullScreen(CCommandBuffer::Ptr vCommandBuffer)
{
    _bindVertexBuffer(vCommandBuffer);
    vCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
}

void CRenderPassFullScreenGeneral::__createVertexBuffer()
{
     __generateScene();

    if (!m_PointDataSet.empty())
    {
        m_pVertexBuffer = make<vk::CVertexBufferTyped<SFullScreenPointData>>();
        m_pVertexBuffer->create(m_pDevice, m_PointDataSet);
    }
}

void CRenderPassFullScreenGeneral::__generateScene()
{
    m_PointDataSet =
    {
        SFullScreenPointData{glm::vec2(-1.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(3.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(-1.0f, 3.0f)},
    };
}
