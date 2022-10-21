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
    if (isValid() && (vUpdateState.RenderpassUpdated || vUpdateState.ImageNum.IsUpdated))
    {
        __createFramebuffers();

        if (m_pPipeline)
        {
            __createPipeline();
        }
    }
}

std::vector<VkCommandBuffer> CRenderPassFullScreen::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline->isValid());
    _ASSERTE(m_FramebufferSet.isValid(vImageIndex));

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    VkClearValue ClearValue = {};
    ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, { ClearValue });
    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_pPipeline->bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, uint32_t(VertexNum), 1, 0, 0);
    }
    
    end();
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

void CRenderPassFullScreen::__createPipeline()
{
    _ASSERTE(isValid() && m_pPipeline);
    m_pPipeline->create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
    m_pPipeline->setImageNum(m_AppInfo.ImageNum);

    for (const auto& Callback : m_PipelineCreateCallbackSet)
        Callback();
}

void CRenderPassFullScreen::__createFramebuffers()
{
    if (!isValid()) return;

    m_FramebufferSet.destroyAndClearAll();

    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.init(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i)
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
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
        m_pVertexBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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