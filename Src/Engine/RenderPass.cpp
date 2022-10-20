#include "RenderPass.h"

using namespace vk;

IRenderPass::IRenderPass()
{
}

void IRenderPass::init(const vk::SAppInfo& vAppInfo)
{
    m_AppInfo = vAppInfo;

    auto PortDesc = _getPortDescV();
    m_pPortSet = make<CPortSet>(PortDesc);
    m_CurPassDesc = _getRenderPassDescV();

    m_pPortSet->hookLinkUpdate([this](ILinkEvent::EventId_t vEventId, ILinkEvent::CPtr vFrom)
        {
            CRenderPassDescriptor NewDesc = _getRenderPassDescV();
            if (NewDesc != m_CurPassDesc) // change happens
            {
                m_CurPassDesc = NewDesc;
                __createRenderpass();
            }
            __triggerRenderpassUpdate();
        }
    );

    // FIXME: init first or create pass/command first
    _initV();

    __createCommandPoolAndBuffers();
    __createRenderpass();
}

void IRenderPass::updateImageInfo(VkFormat vImageFormat, VkExtent2D vImageExtent, size_t vImageNum)
{
    VkFormat OldImageFormat = m_AppInfo.ImageFormat;
    VkExtent2D OldImageExtent = m_AppInfo.Extent;
    size_t OldImageNum = m_AppInfo.ImageNum;

    bool ShouldCommandBufferUpdate = (m_AppInfo.ImageNum != vImageNum);
    m_AppInfo.ImageFormat = vImageFormat;
    m_AppInfo.Extent = vImageExtent;
    m_AppInfo.ImageNum = vImageNum;
    if (ShouldCommandBufferUpdate) __createCommandPoolAndBuffers();

    __triggerImageUpdate(OldImageFormat, OldImageExtent, OldImageNum);
}

void IRenderPass::update(uint32_t vImageIndex)
{
    _ASSERTE(isValid());
    _updateV(vImageIndex);
} 

std::vector<VkCommandBuffer> IRenderPass::requestCommandBuffers(uint32_t vImageIndex)
{
    _ASSERTE(isValid());
    return _requestCommandBuffersV(vImageIndex);
}

void IRenderPass::destroy()
{
    _destroyV();
    __destroyRenderpass();
    __destroyCommandPoolAndBuffers();
}

void IRenderPass::begin(VkCommandBuffer vCommandBuffer, VkFramebuffer vFrameBuffer, VkExtent2D vRenderExtent, const std::vector<VkClearValue>& vClearValues)
{
    _ASSERTE(!m_Begined);
    _ASSERTE(isValid());

    // only need one command one pass, so begin/end command buffer at same time with renderpass
    __beginCommand(vCommandBuffer);

    VkRenderPassBeginInfo RenderPassBeginInfo = {};
    RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RenderPassBeginInfo.renderPass = get();
    RenderPassBeginInfo.framebuffer = vFrameBuffer;
    RenderPassBeginInfo.renderArea.offset = { 0, 0 };
    RenderPassBeginInfo.renderArea.extent = vRenderExtent;
    RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(vClearValues.size());
    RenderPassBeginInfo.pClearValues = vClearValues.data();

    vkCmdBeginRenderPass(vCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    m_CurrentCommandBuffer = vCommandBuffer;
    m_Begined = true;
}

void IRenderPass::end()
{
    _ASSERTE(m_Begined);
    vkCmdEndRenderPass(m_CurrentCommandBuffer);
    __endCommand(m_CurrentCommandBuffer);
    m_CurrentCommandBuffer = VK_NULL_HANDLE;
    m_Begined = false;
}

void IRenderPass::__createCommandPoolAndBuffers()
{
    __destroyCommandPoolAndBuffers();
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_DefaultCommandName, static_cast<uint32_t>(m_AppInfo.ImageNum), ECommandBufferLevel::PRIMARY);
    __triggerCommandUpdate();
}

void IRenderPass::__destroyCommandPoolAndBuffers()
{
    m_Command.clear();
}

bool IRenderPass::__createRenderpass()
{
    __destroyRenderpass();

    if (!m_CurPassDesc.isValid()) return false;

    auto Info = m_CurPassDesc.generateInfo();
    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
    m_CurPassDesc.clearStage(); // free stage data to save memory

#ifdef _DEBUG
    static int Count = 0;
    std::cout << "create renderpass [" << Count << "] = 0x" << std::setbase(16) << (uint64_t)(get()) << std::setbase(10) << std::endl;
    Count++;
#endif

    __triggerRenderpassUpdate();
}

void IRenderPass::__destroyRenderpass()
{
    if (isValid())
    {
        vkDestroyRenderPass(*m_AppInfo.pDevice, get(), nullptr);
        _setNull();
    }
}

void IRenderPass::__beginCommand(VkCommandBuffer vCommandBuffer)
{
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; // TODO: more flag options?

    vk::checkError(vkBeginCommandBuffer(vCommandBuffer, &CommandBufferBeginInfo));
}


void IRenderPass::__endCommand(VkCommandBuffer vCommandBuffer)
{
    vk::checkError(vkEndCommandBuffer(vCommandBuffer));
}