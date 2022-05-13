#include "RenderPass.h"

using namespace vk;

VkAttachmentDescription IRenderPass::createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, EImageType vType)
{
    // TODO: handle loadop to match render pass port, or always use load? but how to clear?

    VkAttachmentStoreOp StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

    VkImageLayout OptimalLayout;
    switch (vType)
    {
    case EImageType::COLOR:
        OptimalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        break;
    case EImageType::DEPTH:
        OptimalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
        break;
    default:
        break;
    }

    VkAttachmentLoadOp LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    
    VkImageLayout InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (vRendererPosBitField & ERenderPassPos::BEGIN)
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
        InitImageLayout = OptimalLayout;
    }

    if (vRendererPosBitField & ERenderPassPos::END)
    {
        FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    else
    {
        FinalImageLayout = OptimalLayout;
    }

    // create renderpass
    VkAttachmentDescription Attachment = {};
    Attachment.format = vImageFormat;
    Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    Attachment.loadOp = LoadOp;
    Attachment.storeOp = StoreOp;
    Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachment.initialLayout = InitImageLayout;
    Attachment.finalLayout = FinalImageLayout;

    return Attachment;
}

void IRenderPass::init(const vk::SAppInfo& vAppInfo, int vRenderPassPosBitField)
{
    m_AppInfo = vAppInfo;
    m_RenderPassPosBitField = vRenderPassPosBitField;

    auto Ports = _getPortV();
    m_pLink = make<CRenderPassLink>(Ports);
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
    return _requestCommandBuffersV(vImageIndex);
}

void IRenderPass::destroy()
{
    _destroyV();
    if (get())
    {
        vkDestroyRenderPass(*m_AppInfo.pDevice, get(), nullptr);
        _setNull();
    }
    m_RenderPassPosBitField = (int)ERenderPassPos::MIDDLE;
}

void IRenderPass::begin(VkCommandBuffer vCommandBuffer, VkFramebuffer vFrameBuffer, VkExtent2D vRenderExtent, const std::vector<VkClearValue>& vClearValues)
{
    _ASSERTE(!m_Begined);

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
    m_CurrentCommandBuffer = VK_NULL_HANDLE;
    m_Begined = false;
}