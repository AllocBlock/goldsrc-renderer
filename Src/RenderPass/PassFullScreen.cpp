#include "PassFullScreen.h"
#include "Function.h"
#include "RenderPassDescriptor.h"

void CRenderPassFullScreen::_initV()
{
    __createVertexBuffer();
}

void CRenderPassFullScreen::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
}

CRenderPassDescriptor CRenderPassFullScreen::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassFullScreen::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    CRenderPassSingle::_onUpdateV(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (!_dumpInputPortExtent("Main", RefExtent)) return;

    if (isValid() && (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated))
    {
        __createPipeline(RefExtent);
    }
}

std::vector<VkCommandBuffer> CRenderPassFullScreen::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline->isValid());

    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    _beginWithFramebuffer(vImageIndex);

    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_pPipeline->bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }
    
    _endWithFramebuffer();
    return { CommandBuffer };
}

void CRenderPassFullScreen::_destroyV()
{
    destroyAndClear(m_pPipeline);
    destroyAndClear(m_pVertexBuffer);

    CRenderPassSingle::_destroyV();
}

void CRenderPassFullScreen::__createPipeline(VkExtent2D vExtent)
{
    _ASSERTE(isValid() && m_pPipeline);
    m_pPipeline->create(m_pDevice, get(), vExtent);
    m_pPipeline->setImageNum(m_pAppInfo->getImageNum());

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
        SFullScreenPointData{glm::vec2(3.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(-1.0f, 3.0f)},
    };
}

void CRenderPassFullScreenGeneral::_initV()
{
    __createVertexBuffer();
    _initPipelineV();
}

void CRenderPassFullScreenGeneral::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    VkExtent2D Extent;
    if (!_dumpReferenceExtentV(Extent)) return;

    if (isValid() && (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated))
    {
        __createPipeline(Extent);
    }
}

std::vector<VkCommandBuffer> CRenderPassFullScreenGeneral::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline->isValid());

    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    _beginWithFramebuffer(vImageIndex);

    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_pPipeline->bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }
    
    _endWithFramebuffer();
    return { CommandBuffer };
}

void CRenderPassFullScreenGeneral::_destroyV()
{
    destroyAndClear(m_pPipeline);
    destroyAndClear(m_pVertexBuffer);

    IRenderPass::_destroyV();
}

void CRenderPassFullScreenGeneral::__createPipeline(VkExtent2D vExtent)
{
    _ASSERTE(isValid() && m_pPipeline);
    m_pPipeline->create(m_pDevice, get(), vExtent);
    m_pPipeline->setImageNum(m_pAppInfo->getImageNum());
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
