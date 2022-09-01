#include "RenderPassFullScreen.h"
#include "Function.h"
#include "RenderPassDescriptor.h"

void CRenderPassFullScreen::_initV()
{
    __createCommandPoolAndBuffers();
    __createVertexBuffer();
    __createRecreateResources();
}

SPortDescriptor CRenderPassFullScreen::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addOutput("Output");
    return Ports;
}

CRenderPassDescriptor CRenderPassFullScreen::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Output"));
}

void CRenderPassFullScreen::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

std::vector<VkCommandBuffer> CRenderPassFullScreen::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(m_pPipeline);
    //_ASSERTE(m_pPipeline->isReady());

    if (m_FramebufferSet.empty() || m_IsUpdated)
        __createFramebuffers();

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

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
    __destroyRecreateResources();
    if (m_pPipeline) m_pPipeline->destroy();
    m_pVertexBuffer->destroy();
    m_Command.clear();

    IRenderPass::_destroyV();
}

void CRenderPassFullScreen::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_CommandName, uint32_t(m_AppInfo.ImageNum), ECommandBufferLevel::PRIMARY);
}

void CRenderPassFullScreen::__createFramebuffers()
{
    _ASSERTE(isValid());

    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Output")->getImageV(i)
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
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

void CRenderPassFullScreen::__createRecreateResources()
{
    if (m_pPipeline)
        m_pPipeline->setImageNum(m_AppInfo.ImageNum);
}

void CRenderPassFullScreen::__destroyRecreateResources()
{
    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
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