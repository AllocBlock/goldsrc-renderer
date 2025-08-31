#include "RenderInfoDescriptor.h"

void CRenderInfoDescriptor::addColorAttachment(VkImageView vImage, VkFormat vFormat, VkImageLayout vLayout, bool vIsBegin, bool vIsEnd)
{
    // if it's begin clear it, otherwise load it
    VkAttachmentLoadOp LoadOp = vIsBegin ? VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR : VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

    VkRenderingAttachmentInfo Info = {};
    Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    Info.imageView = vImage;
    Info.imageLayout = vLayout;
    Info.loadOp = LoadOp;
    Info.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    Info.clearValue = kDefaultClearValueColor;

    m_AttachmentInfoColors.emplace_back(Info);
    m_AttachmentFormatColors.emplace_back(vFormat);
}

void CRenderInfoDescriptor::setDepthAttachment(VkImageView vImage, VkFormat vFormat, VkImageLayout vLayout, bool vIsBegin, bool vIsEnd)
{
    // if it's begin clear it, otherwise load it
    VkAttachmentLoadOp LoadOp = vIsBegin ? VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR : VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

    VkRenderingAttachmentInfo Info = {};
    Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    Info.imageView = vImage;
    Info.imageLayout = vLayout;
    Info.loadOp = LoadOp;
    Info.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    Info.clearValue = kDefaultClearValueDepth;

    m_AttachmentInfoDepthStencil = Info;
    m_AttachmentFormatDepthStencil = vFormat;
}

void CRenderInfoDescriptor::addColorAttachment(CPort::Ptr vPort)
{
    bool IsBegin = vPort->isRoot();
    bool IsEnd = vPort->isTail();
    addColorAttachment(*vPort->getImageV(), vPort->getImageV()->getFormat(), vPort->getLayout(), IsBegin, IsEnd);
}

void CRenderInfoDescriptor::setDepthAttachment(CPort::Ptr vPort)
{
    bool IsBegin = vPort->isRoot();
    bool IsEnd = vPort->isTail();
    setDepthAttachment(*vPort->getImageV(), vPort->getImageV()->getFormat(), vPort->getLayout(), IsBegin, IsEnd);
}

void CRenderInfoDescriptor::clear()
{
    m_AttachmentInfoColors.clear();
    m_AttachmentInfoDepthStencil = std::nullopt;
    //m_StageSubpassDescSet.clear();
    //m_StageDepedencySet.clear();
    //m_StageColorRefSetSet.clear();
    //m_StageDepthRefSet.clear();
}

const std::vector<VkFormat>& CRenderInfoDescriptor::getColorAttachmentFormats() const
{
    return m_AttachmentFormatColors;
}

bool CRenderInfoDescriptor::hasDepthAttachment() const
{
    return m_AttachmentInfoDepthStencil.has_value() && m_AttachmentFormatDepthStencil.has_value();
}

VkFormat CRenderInfoDescriptor::getDepthAttachmentFormat() const
{
    return m_AttachmentFormatDepthStencil.value();
}

VkRenderingInfo CRenderInfoDescriptor::generateRendererInfo(VkExtent2D renderSize, bool vHasSecondary)
{
    VkRenderingInfo RenderingInfo = {};
    RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    RenderingInfo.flags = vHasSecondary ? VkRenderingFlagBits::VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT : 0;
    RenderingInfo.renderArea = VkRect2D{ VkOffset2D{}, renderSize };
    RenderingInfo.layerCount = 1;

    RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(m_AttachmentInfoColors.size());
    RenderingInfo.pColorAttachments = m_AttachmentInfoColors.data();
    if (m_AttachmentInfoDepthStencil.has_value())
    {
        RenderingInfo.pDepthAttachment = &m_AttachmentInfoDepthStencil.value();
        RenderingInfo.pStencilAttachment = &m_AttachmentInfoDepthStencil.value();
    }

    return std::move(RenderingInfo);
}

CRenderInfoDescriptor CRenderInfoDescriptor::generateSingleSubpassDesc(CPort::Ptr vColorPort, CPort::Ptr vDepthPort)
{
    CRenderInfoDescriptor Desc;
    Desc.addColorAttachment(vColorPort);
    if (vDepthPort)
        Desc.setDepthAttachment(vDepthPort);
    return Desc;
}

//void CRenderInfoDescriptor::__generateDependency()
//{
//    // about subpass dependency
//    // https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/
//    std::vector<std::pair<uint32_t, uint32_t>> DepSet;
//    if (m_SubpassReferenceInfoSet.empty())
//    {
//        DepSet.emplace_back(std::make_pair(VK_SUBPASS_EXTERNAL, 0));
//    }
//    else
//    {
//        for (size_t i = 0; i < m_SubpassReferenceInfoSet.size(); ++i)
//        {
//            for (uint32_t vDepPass : m_SubpassReferenceInfoSet[i].DependentPassIndices)
//                DepSet.emplace_back(std::make_pair(vDepPass, i));
//        }
//    }
//    m_StageDepedencySet.resize(DepSet.size());
//    for (uint32_t i = 0; i < DepSet.size(); ++i)
//    {
//        m_StageDepedencySet[i].srcSubpass = DepSet[i].first;
//        m_StageDepedencySet[i].dstSubpass = DepSet[i].second;
//
//        // TODO: to imporve performance, should not use ALL
//        m_StageDepedencySet[i].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
//        m_StageDepedencySet[i].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
//
//        VkAccessFlags SrcAcccesFlags = 0;
//        VkAccessFlags DestAcccesFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//        if (DepSet[i].first != VK_SUBPASS_EXTERNAL)
//        {
//            SrcAcccesFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//            DestAcccesFlags |= VK_ACCESS_SHADER_READ_BIT;
//        }
//        m_StageDepedencySet[i].srcAccessMask = SrcAcccesFlags;
//        m_StageDepedencySet[i].dstAccessMask = DestAcccesFlags;
//    }
//}

//VkSubpassDescription CRenderInfoDescriptor::__generateSubpassDescriptionFromTargetInfo(const SSubpassReferenceInfo& vTargetInfo)
//{
//    VkSubpassDescription SubpassDesc = {};
//
//    m_StageInputRefSetSet.push_back({});
//    auto& StageInputRefSet = m_StageInputRefSetSet[m_StageInputRefSetSet.size() - 1];
//    uint32_t InputNum = static_cast<uint32_t>(vTargetInfo.InputIndices.size());
//    StageInputRefSet.resize(InputNum);
//    for (size_t i = 0; i < vTargetInfo.InputIndices.size(); ++i)
//    {
//        const SAttachmentReference& AttachmentRef = vTargetInfo.InputIndices[i];
//        _ASSERTE(AttachmentRef.Index <= getAttachementNum());
//        StageInputRefSet[i].attachment = AttachmentRef.Index;
//        StageInputRefSet[i].layout = AttachmentRef.Layout;
//    }
//    SubpassDesc.inputAttachmentCount = InputNum;
//    SubpassDesc.pInputAttachments = StageInputRefSet.data();
//
//    m_StageColorRefSetSet.push_back({});
//    auto& StageColorRefSet = m_StageColorRefSetSet[m_StageColorRefSetSet.size() - 1];
//    uint32_t ColorAttachementNum = static_cast<uint32_t>(vTargetInfo.ColorIndices.size());
//    StageColorRefSet.resize(ColorAttachementNum);
//    for (size_t i = 0; i < vTargetInfo.ColorIndices.size(); ++i)
//    {
//        const SAttachmentReference& AttachmentRef = vTargetInfo.ColorIndices[i];
//        _ASSERTE(AttachmentRef.Index < m_ColorAttachmentInfoSet.size());
//        StageColorRefSet[i].attachment = AttachmentRef.Index;
//        StageColorRefSet[i].layout = AttachmentRef.Layout;
//    }
//    SubpassDesc.colorAttachmentCount = ColorAttachementNum;
//    SubpassDesc.pColorAttachments = StageColorRefSet.data();
//
//    _ASSERTE(!vTargetInfo.UseDepth || m_DepthAttachmentInfo.has_value());
//    uint32_t DepthAttachmentIndex = m_ColorAttachmentInfoSet.size();
//    if (m_DepthAttachmentInfo.has_value())
//    {
//        m_StageDepthRefSet.push_back({ DepthAttachmentIndex , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }); // TODO: custom layout
//        auto& StageDepthRef = m_StageDepthRefSet[m_StageDepthRefSet.size() - 1];
//        SubpassDesc.pDepthStencilAttachment = &StageDepthRef;
//    }
//    else
//    {
//        SubpassDesc.pDepthStencilAttachment = nullptr;
//    }
//    return SubpassDesc;
//}

//void CRenderInfoDescriptor::__generateSubpassDescription()
//{
//    m_StageSubpassDescSet.clear();
//    if (m_SubpassReferenceInfoSet.empty()) // did not specify, all attachement are used
//    {
//        SSubpassReferenceInfo Info;
//        for (size_t i = 0; i < m_ColorAttachmentInfoSet.size(); ++i)
//            Info.addColorRef(i);
//        Info.setUseDepth(m_DepthAttachmentInfo.has_value());
//            
//        VkSubpassDescription CommonDesc = __generateSubpassDescriptionFromTargetInfo(Info);
//        m_StageSubpassDescSet.resize(1, CommonDesc);
//    }
//    else
//    {
//        for (const auto& Info : m_SubpassReferenceInfoSet)
//        {
//            VkSubpassDescription Desc = __generateSubpassDescriptionFromTargetInfo(Info);
//            m_StageSubpassDescSet.emplace_back(Desc);
//        }
//    }
//}