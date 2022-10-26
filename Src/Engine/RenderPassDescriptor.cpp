#include "RenderPassDescriptor.h"

SAttachementInfo CRenderPassDescriptor::__getAttachmentInfo(CPort::Ptr vPort, bool vIsDepth)
{
    if (!vPort->isLinkReadyV() || !vPort->hasActualFormatV())
    {
        clear();
        m_IsValid = false;
        return SAttachementInfo();
    }

    bool IsBegin = (vPort->isRoot() || (vPort->hasParent() && vPort->getParent()->isSwapchainSource()));
    bool IsEnd = vPort->isTail();

    EUsage CurUsage = EUsage::UNDEFINED;
    if (!IsBegin)
    {
        CurUsage = vPort->getFormat().Usage;
    }

    EUsage NextUsage = EUsage::PRESENTATION;
    if (!IsEnd)
    {
        _ASSERTE(vPort->getChildNum() > 0);
        NextUsage = vPort->getChild(0)->getFormat().Usage;
        for (size_t i = 1; i < vPort->getChildNum(); ++i)
        {
            if (NextUsage != vPort->getChild(i)->getFormat().Usage)
            {
                throw std::runtime_error("Error: Output port connects to multiple ports with different layout requirments!");
            }
        }
    }

    return { vPort->getActualFormatV(), toLayout(CurUsage, vIsDepth), toLayout(NextUsage, vIsDepth), IsBegin, IsEnd };
}

void CRenderPassDescriptor::addColorAttachment(CPort::Ptr vPort)
{
    addColorAttachment(__getAttachmentInfo(vPort, false));
}

void CRenderPassDescriptor::setDepthAttachment(CPort::Ptr vPort)
{
    setDepthAttachment(__getAttachmentInfo(vPort, true));
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
    VkAttachmentStoreOp StoreOp = vIsDepth ? VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE : VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

    // if it's begin clear it, otherwise load it
    VkAttachmentLoadOp LoadOp = vInfo.IsBegin ? VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR : VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

    VkImageLayout InitImageLayout = vInfo.InitLayout;
    VkImageLayout FinalImageLayout = vInfo.FinalLayout;

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