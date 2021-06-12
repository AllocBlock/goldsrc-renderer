#include "Renderer.h"

VkAttachmentDescription CRenderer::createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, EImageType vType)
{
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
    if (vRendererPosBitField & ERendererPos::BEGIN)
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
        InitImageLayout = OptimalLayout;
    }

    if (vRendererPosBitField & ERendererPos::END)
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

void CRenderer::init(const Common::SVulkanAppInfo& vAppInfo, int vRenderPassPosBitField)
{
    m_AppInfo = vAppInfo;
    m_RenderPassPosBitField = vRenderPassPosBitField;
    _initV();
}

void CRenderer::recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    _recreateV();
}

void CRenderer::update(uint32_t vImageIndex)
{
    _updateV(vImageIndex);
}

VkCommandBuffer CRenderer::requestCommandBuffer(uint32_t vImageIndex)
{
    return _requestCommandBufferV(vImageIndex);
}

void CRenderer::destroy()
{
    _destroyV();
    m_RenderPassPosBitField = 0;
    m_AppInfo.clear();
}