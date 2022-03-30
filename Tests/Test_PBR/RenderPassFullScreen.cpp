#include "RenderPassFullScreen.h"
#include "UserInterface.h"
#include "Function.h"

void CRenderPassFullScreen::_initV()
{
    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createVertexBuffer();
    __createRecreateResources();
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

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    Vulkan::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    std::array<VkClearValue, 1> ClearValues = {};
    ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = m_RenderPass;
    RenderPassBeginInfo.framebuffer = m_FramebufferSet[vImageIndex]->get();
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
    RenderPassBeginInfo.pClearValues = ClearValues.data();

    vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = m_pVertexBuffer->get();
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_pPipeline->bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, VertexNum, 1, 0, 0);
    }
    
    vkCmdEndRenderPass(CommandBuffer);
    Vulkan::checkError(vkEndCommandBuffer(CommandBuffer));
    return { CommandBuffer };
}

void CRenderPassFullScreen::_destroyV()
{
    __destroyRecreateResources();
    m_pVertexBuffer->destroy();
    vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
    m_Command.clear();

    IRenderPass::_destroyV();
}

void CRenderPassFullScreen::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = IRenderPass::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDependency, 1> SubpassDependencies = {};
    SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependencies[0].dstSubpass = 0;
    SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].srcAccessMask = 0;
    SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColorAttachmentRef;

    std::vector<VkSubpassDescription> SubpassDescs = { SubpassDesc };

    std::vector<VkAttachmentDescription> Attachments = { ColorAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(SubpassDescs.size());
    RenderPassInfo.pSubpasses = SubpassDescs.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(SubpassDependencies.size());
    RenderPassInfo.pDependencies = SubpassDependencies.data();

    Vulkan::checkError(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CRenderPassFullScreen::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CRenderPassFullScreen::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_CommandName, m_AppInfo.TargetImageViewSet.size(), ECommandBufferLevel::PRIMARY);
}

void CRenderPassFullScreen::__createFramebuffers()
{
    size_t ImageNum = m_AppInfo.TargetImageViewSet.size();
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_AppInfo.TargetImageViewSet[i]
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.Device, m_RenderPass, AttachmentSet, m_AppInfo.Extent);
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
        m_pVertexBuffer->create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}

void CRenderPassFullScreen::__createRecreateResources()
{
    __createFramebuffers();
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