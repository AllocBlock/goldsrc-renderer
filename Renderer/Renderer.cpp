#include "Renderer.h"

void CRenderer::init(GLFWwindow* vWindow, const Common::SVulkanAppInfo& vAppInfo, int vRenderPassPosBitField)
{
    m_RenderPass = VK_NULL_HANDLE;
    m_pWindow = vWindow;
    m_AppInfo = vAppInfo;
    m_RenderPassPosBitField = vRenderPassPosBitField;
    __createRenderPass();

    _initV();
}

void CRenderer::recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    __destroyRenderPass();
    __createRenderPass();

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
    __destroyRenderPass();
    m_RenderPassPosBitField = 0;
    m_RenderPass = VK_NULL_HANDLE;
    m_pWindow = nullptr;
    m_AppInfo.clear();
}

void CRenderer::__createRenderPass()
{
    VkAttachmentLoadOp LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkImageLayout InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (m_RenderPassPosBitField & ERendererPos::BEGIN)
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
        InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    if (m_RenderPassPosBitField & ERendererPos::END)
    {
        FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    else
    {
        FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // create renderpass
    VkAttachmentDescription Attachment = {};
    Attachment.format = m_AppInfo.ImageFormat;
    Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    Attachment.loadOp = LoadOp;
    Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachment.initialLayout = InitImageLayout;
    Attachment.finalLayout = FinalImageLayout;

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription Subpass = {};
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments = &ColorAttachmentRef;

    VkSubpassDependency SubpassDependency = {}; // for render pass sync
    SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependency.dstSubpass = 0;
    SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependency.srcAccessMask = 0;
    SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = 1;
    RenderPassInfo.pAttachments = &Attachment;
    RenderPassInfo.subpassCount = 1;
    RenderPassInfo.pSubpasses = &Subpass;
    RenderPassInfo.dependencyCount = 1;
    RenderPassInfo.pDependencies = &SubpassDependency;

    ck(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CRenderer::__destroyRenderPass()
{
    vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
}