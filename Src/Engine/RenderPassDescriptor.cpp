#include "RenderPassDescriptor.h"

CRenderPassDescriptor CRenderPassDescriptor::gGlobalDesc = CRenderPassDescriptor();

void CRenderPassDescriptor::addColorAttachment(int vRendererPosBitField, VkFormat vImageFormat)
{
    addColorAttachment(__createAttachmentDescription(vRendererPosBitField, vImageFormat, false));
}

void CRenderPassDescriptor::setDepthAttachment(int vRendererPosBitField, VkFormat vImageFormat)
{
    setDepthAttachment(__createAttachmentDescription(vRendererPosBitField, vImageFormat, true));
}

void CRenderPassDescriptor::addColorAttachment(const VkAttachmentDescription& vDesc)
{
    _ASSERTE(!m_HasDepthAttachment); // should not set color after depth, so the depth is always the last one
    m_ColorAttachmentNum++;
    m_AttachmentDescSet.emplace_back(vDesc);
}

void CRenderPassDescriptor::setDepthAttachment(const VkAttachmentDescription& vDesc)
{
    _ASSERTE(!m_HasDepthAttachment); // should not set depth again
    m_HasDepthAttachment = true;
    m_AttachmentDescSet.emplace_back(vDesc);
}

void CRenderPassDescriptor::setSubpassNum(uint32_t vNum)
{
    _ASSERTE(vNum >= 1);
    m_SubPassNum = vNum;
}

void CRenderPassDescriptor::clear()
{
    m_SubPassNum = 1;
    m_AttachmentDescSet.clear();
    m_ColorAttachmentNum = 0;
    m_HasDepthAttachment = false;
}

VkRenderPassCreateInfo CRenderPassDescriptor::generateInfo()
{
    __generateDescription();
    __generateDependency();

    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(m_AttachmentDescSet.size());
    RenderPassInfo.pAttachments = m_AttachmentDescSet.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(m_StageDescSet.size());
    RenderPassInfo.pSubpasses = m_StageDescSet.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(m_StageDepedencySet.size());
    RenderPassInfo.pDependencies = m_StageDepedencySet.data();

    return std::move(RenderPassInfo);
}

VkRenderPassCreateInfo CRenderPassDescriptor::generateSingleSubpassInfo(int vRendererPosBitField, VkFormat vColorImageFormat, VkFormat vDepthImageFormat)
{
    gGlobalDesc.clear();
    gGlobalDesc.addColorAttachment(vRendererPosBitField, vColorImageFormat);
    if (vDepthImageFormat != VK_FORMAT_UNDEFINED)
        gGlobalDesc.setDepthAttachment(vRendererPosBitField, vDepthImageFormat);
    return std::move(gGlobalDesc.generateInfo());
}

VkAttachmentDescription CRenderPassDescriptor::__createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, bool vIsDepth)
{
    // TODO: handle loadop to match render pass port, or always use load? but how to clear?
    VkAttachmentStoreOp StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout OptimalLayout;

    if (!vIsDepth)
    {
        OptimalLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    }
    else
    {
        OptimalLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }

    VkAttachmentLoadOp LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;

    VkImageLayout InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    if (vRendererPosBitField & vk::ERenderPassPos::BEGIN)
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
        InitImageLayout = OptimalLayout;
    }

    if (vRendererPosBitField & vk::ERenderPassPos::END)
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

void CRenderPassDescriptor::__generateDependency()
{
    m_StageDepedencySet.resize(m_SubPassNum);
    for (uint32_t i = 0; i < m_SubPassNum; ++i)
    {
        m_StageDepedencySet[i].srcSubpass = i == 0 ? VK_SUBPASS_EXTERNAL : i - 1;
        m_StageDepedencySet[i].dstSubpass = i;

        m_StageDepedencySet[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        m_StageDepedencySet[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        m_StageDepedencySet[i].srcAccessMask = 0;
        m_StageDepedencySet[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
}

void CRenderPassDescriptor::__generateDescription()
{
    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    m_StageColorRefSet.resize(m_ColorAttachmentNum);
    for (size_t i = 0; i < m_ColorAttachmentNum; ++i)
    {
        m_StageColorRefSet[i].attachment = i;
        m_StageColorRefSet[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    SubpassDesc.colorAttachmentCount = m_ColorAttachmentNum;
    SubpassDesc.pColorAttachments = m_StageColorRefSet.data();

    if (m_HasDepthAttachment)
    {
        uint32_t DepthAttachmentIndex = m_ColorAttachmentNum;
        m_StageDepthRef.attachment = DepthAttachmentIndex;
        m_StageDepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        SubpassDesc.pDepthStencilAttachment = &m_StageDepthRef;
    }
    else
    {
        SubpassDesc.pDepthStencilAttachment = nullptr;
    }

    m_StageDescSet.clear();
    m_StageDescSet.resize(m_SubPassNum, SubpassDesc);
}