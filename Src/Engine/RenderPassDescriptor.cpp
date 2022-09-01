#include "RenderPassDescriptor.h"

SAttachementInfo CRenderPassDescriptor::__getAttachmentInfo(CPort::Ptr vPort)
{
    if (!vPort->isReadyV() || !vPort->hasActualFormatV())
    {
        clear();
        m_IsValid = false;
        return SAttachementInfo();
    }

    bool isBegin = (vPort->isRoot() || (vPort->hasParent() && vPort->getParent()->isSwapchainSource()));
    bool isEnd = vPort->isTail();

    return { vPort->getActualFormatV().Format, isBegin, isEnd };
}

void CRenderPassDescriptor::addColorAttachment(CPort::Ptr vPort)
{
    addColorAttachment(__getAttachmentInfo(vPort));
}

void CRenderPassDescriptor::setDepthAttachment(CPort::Ptr vPort)
{
    setDepthAttachment(__getAttachmentInfo(vPort));
}

void CRenderPassDescriptor::addColorAttachment(const SAttachementInfo& vInfo)
{
    m_ColorAttachmentInfoSet.emplace_back(vInfo);
}

void CRenderPassDescriptor::setDepthAttachment(const SAttachementInfo& vInfo)
{
    m_DepthAttachmentInfo = vInfo;
}

void CRenderPassDescriptor::setSubpassNum(uint32_t vNum)
{
    _ASSERTE(vNum >= 1);
    m_SubPassNum = vNum;
}

void CRenderPassDescriptor::clear()
{
    m_SubPassNum = 1;
    m_ColorAttachmentInfoSet.clear();
    m_DepthAttachmentInfo = std::nullopt;
    clearStage();
}

void CRenderPassDescriptor::clearStage()
{
    m_StageAttachmentDescSet.clear();
    m_StageSubpassDescSet.clear();
    m_StageDepedencySet.clear();
    m_StageColorRefSet.clear();
    m_StageDepthRef = VkAttachmentReference();
}

VkRenderPassCreateInfo CRenderPassDescriptor::generateInfo()
{
    __generateAttachmentDescription();
    __generateSubpassDescription();
    __generateDependency();

    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(m_StageAttachmentDescSet.size());
    RenderPassInfo.pAttachments = m_StageAttachmentDescSet.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(m_StageSubpassDescSet.size());
    RenderPassInfo.pSubpasses = m_StageSubpassDescSet.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(m_StageDepedencySet.size());
    RenderPassInfo.pDependencies = m_StageDepedencySet.data();

    return std::move(RenderPassInfo);
}

CRenderPassDescriptor CRenderPassDescriptor::generateSingleSubpassDesc(CPort::Ptr vColorPort, CPort::Ptr vDepthPort)
{
    CRenderPassDescriptor Desc;
    Desc.addColorAttachment(vColorPort);
    if (vDepthPort)
        Desc.setDepthAttachment(vDepthPort);
    return Desc;
}

VkAttachmentDescription CRenderPassDescriptor::__createAttachmentDescription(const SAttachementInfo& vInfo, bool vIsDepth)
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

    // if it's begin clear it, otherwise load it
    if (vInfo.IsBegin)
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        InitImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    }
    else
    {
        LoadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;
        InitImageLayout = OptimalLayout;
    }

    // if it's end to present layout, otherwise too optimized layout
    if (vInfo.IsEnd)
    {
        FinalImageLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    else
    {
        FinalImageLayout = OptimalLayout;
    }

    // create renderpass
    VkAttachmentDescription Attachment = {};
    Attachment.format = vInfo.Format;
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

void CRenderPassDescriptor::__generateAttachmentDescription()
{
    for (const auto& Info : m_ColorAttachmentInfoSet)
    {
        m_StageAttachmentDescSet.emplace_back(__createAttachmentDescription(Info, false));
    }

    if (m_DepthAttachmentInfo.has_value())
        m_StageAttachmentDescSet.emplace_back(__createAttachmentDescription(m_DepthAttachmentInfo.value(), true));
}

void CRenderPassDescriptor::__generateSubpassDescription()
{
    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    uint32_t ColorNum = uint32_t(m_ColorAttachmentInfoSet.size());
    m_StageColorRefSet.resize(ColorNum);
    for (uint32_t i = 0; i < ColorNum; ++i)
    {
        m_StageColorRefSet[i].attachment = i;
        m_StageColorRefSet[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    SubpassDesc.colorAttachmentCount = ColorNum;
    SubpassDesc.pColorAttachments = m_StageColorRefSet.data();

    if (m_DepthAttachmentInfo.has_value())
    {
        uint32_t DepthAttachmentIndex = ColorNum;
        m_StageDepthRef.attachment = DepthAttachmentIndex;
        m_StageDepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        SubpassDesc.pDepthStencilAttachment = &m_StageDepthRef;
    }
    else
    {
        SubpassDesc.pDepthStencilAttachment = nullptr;
    }

    m_StageSubpassDescSet.clear();
    m_StageSubpassDescSet.resize(m_SubPassNum, SubpassDesc);
}