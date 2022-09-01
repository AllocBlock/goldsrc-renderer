#include "IRenderPass.h"

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

    m_pPortSet->hookLinkUpdate([this]()
        {
            CRenderPassDescriptor NewDesc = _getRenderPassDescV();
            if (NewDesc != m_CurPassDesc) // change happens
            {
                m_CurPassDesc = NewDesc;
                __destroyRenderpass();
                __createRenderpass();
            }
        }
    );

    __createRenderpass();
    _initV();
}

void IRenderPass::recreate(VkFormat vImageFormat, VkExtent2D vExtent, size_t vImageNum)
{
    m_AppInfo.ImageFormat = vImageFormat;
    m_AppInfo.Extent = vExtent;
    m_AppInfo.ImageNum = vImageNum;
    _recreateV();
}

void IRenderPass::update(uint32_t vImageIndex)
{
    _updateV(vImageIndex);
} 

std::vector<VkCommandBuffer> IRenderPass::requestCommandBuffers(uint32_t vImageIndex)
{
    //return _requestCommandBuffersV(vImageIndex);
    // FIXME: temp design!
    const auto& Res = _requestCommandBuffersV(vImageIndex);
    m_IsUpdated = false;
    return Res;
}

void IRenderPass::destroy()
{
    _destroyV();
    __destroyRenderpass();
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

void IRenderPass::__createRenderpass()
{
    if (!m_CurPassDesc.isValid()) return;

    auto Info = m_CurPassDesc.generateInfo();
    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
    m_CurPassDesc.clearStage(); // free stage data to save memory

    _onRenderPassRecreateV();
}

void IRenderPass::__destroyRenderpass()
{
    if (get())
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