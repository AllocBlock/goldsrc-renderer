#include "PassFullScreen.h"
#include "Function.h"
#include "RenderPassDescriptor.h"

void CRenderPassFullScreen::_initV()
{
    __createVertexBuffer();
}

SPortDescriptor CRenderPassFullScreen::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    return Ports;
}

CRenderPassDescriptor CRenderPassFullScreen::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"));
}

void CRenderPassFullScreen::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    VkExtent2D RefExtent = { 0, 0 };
    if (!_dumpInputPortExtent("Main", RefExtent)) return;

    if (isValid() && (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated))
    {
        __createFramebuffers(RefExtent);
        __createPipeline(RefExtent);
    }
}

std::vector<VkCommandBuffer> CRenderPassFullScreen::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline->isValid());
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    _begin(CommandBuffer, m_FramebufferSet[vImageIndex], { ClearValue });
    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_pPipeline->bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }
    
    _end();
    return { CommandBuffer };
}

void CRenderPassFullScreen::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    if (m_pPipeline) m_pPipeline->destroy();
    m_pVertexBuffer->destroy();
    m_pVertexBuffer = nullptr;

    IRenderPass::_destroyV();
}

void CRenderPassFullScreen::__createPipeline(VkExtent2D vExtent)
{
    _ASSERTE(isValid() && m_pPipeline);
    m_pPipeline->create(m_pDevice, get(), vExtent);
    m_pPipeline->setImageNum(m_pAppInfo->getImageNum());

    for (const auto& Callback : m_PipelineCreateCallbackSet)
        Callback();
}

void CRenderPassFullScreen::__createFramebuffers(VkExtent2D vExtent)
{
    if (!isValid()) return;

    m_FramebufferSet.destroyAndClearAll();

    size_t ImageNum = m_pAppInfo->getImageNum();
    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i)
        };

        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
    }
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
        __createFramebuffers(Extent);
        __createPipeline(Extent);
    }
}

std::vector<VkCommandBuffer> CRenderPassFullScreenGeneral::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline->isValid());
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    _begin(CommandBuffer, m_FramebufferSet[vImageIndex], { ClearValue });
    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_pPipeline->bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }
    
    _end();
    return { CommandBuffer };
}

void CRenderPassFullScreenGeneral::_destroyV()
{
    m_FramebufferSet.destroyAndClearAll();
    if (m_pPipeline) m_pPipeline->destroy();
    destroyAndClear(m_pVertexBuffer);

    IRenderPass::_destroyV();
}

void CRenderPassFullScreenGeneral::__createPipeline(VkExtent2D vExtent)
{
    _ASSERTE(isValid() && m_pPipeline);
    m_pPipeline->create(m_pDevice, get(), vExtent);
    m_pPipeline->setImageNum(m_pAppInfo->getImageNum());
}

void CRenderPassFullScreenGeneral::__createFramebuffers(VkExtent2D vExtent)
{
    if (!isValid()) return;

    m_FramebufferSet.destroyAndClearAll();

    size_t ImageNum = m_pAppInfo->getImageNum();
    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet = _getAttachmentsV(i);
        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, vExtent);
    }
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