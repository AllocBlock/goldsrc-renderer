#include "RenderPassDescriptor.h"

SAttachementInfo CRenderPassDescriptor::__getAttachmentInfo(CPort::Ptr vPort, bool vIsDepth)
{
    auto pImage = vPort->getImageV();
    VkFormat Format = pImage->getFormat();
    VkImageLayout InputLayout = vPort->getLayout();
    VkImageLayout OutputLayout = vPort->getLayout();
    _ASSERTE(OutputLayout != VK_IMAGE_LAYOUT_UNDEFINED);
    bool IsBegin = vPort->isRoot() || vPort->hasParent();
    bool IsEnd = vPort->isTail();
    
    return { Format, InputLayout, OutputLayout, IsBegin, IsEnd };
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

void CRenderPassDescriptor::addSubpass(const SSubpassReferenceInfo& vSubpassInfo)
{
    m_SubpassReferenceInfoSet.emplace_back(vSubpassInfo);
}

void CRenderPassDescriptor::clear()
{
    m_ColorAttachmentInfoSet.clear();
    m_DepthAttachmentInfo = std::nullopt;
    clearStage();
}

uint32_t CRenderPassDescriptor::getAttachementNum() const
{
    uint32_t Num = static_cast<uint32_t>(m_ColorAttachmentInfoSet.size());
    if (m_DepthAttachmentInfo.has_value())
        Num++;
    return Num;
}

void CRenderPassDescriptor::clearStage()
{
    m_StageAttachmentDescSet.clear();
    m_StageSubpassDescSet.clear();
    m_StageDepedencySet.clear();
    m_StageColorRefSetSet.clear();
    m_StageDepthRefSet.clear();
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
    VkAttachmentStoreOp StoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;

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
    // about subpass dependency
    // https://www.reddit.com/r/vulkan/comments/s80reu/subpass_dependencies_what_are_those_and_why_do_i/
    std::vector<std::pair<uint32_t, uint32_t>> DepSet;
    if (m_SubpassReferenceInfoSet.empty())
    {
        DepSet.emplace_back(std::make_pair(VK_SUBPASS_EXTERNAL, 0));
    }
    else
    {
        for (size_t i = 0; i < m_SubpassReferenceInfoSet.size(); ++i)
        {
            for (uint32_t vDepPass : m_SubpassReferenceInfoSet[i].DependentPassIndices)
                DepSet.emplace_back(std::make_pair(vDepPass, i));
        }
    }
    m_StageDepedencySet.resize(DepSet.size());
    for (uint32_t i = 0; i < DepSet.size(); ++i)
    {
        m_StageDepedencySet[i].srcSubpass = DepSet[i].first;
        m_StageDepedencySet[i].dstSubpass = DepSet[i].second;

        // TODO: to imporve performance, should not use ALL
        m_StageDepedencySet[i].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        m_StageDepedencySet[i].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

        VkAccessFlags SrcAcccesFlags = 0;
        VkAccessFlags DestAcccesFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if (DepSet[i].first != VK_SUBPASS_EXTERNAL)
        {
            SrcAcccesFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            DestAcccesFlags |= VK_ACCESS_SHADER_READ_BIT;
        }
        m_StageDepedencySet[i].srcAccessMask = SrcAcccesFlags;
        m_StageDepedencySet[i].dstAccessMask = DestAcccesFlags;
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

VkSubpassDescription CRenderPassDescriptor::__generateSubpassDescriptionFromTargetInfo(const SSubpassReferenceInfo& vTargetInfo)
{
    VkSubpassDescription SubpassDesc = {};

    m_StageInputRefSetSet.push_back({});
    auto& StageInputRefSet = m_StageInputRefSetSet[m_StageInputRefSetSet.size() - 1];
    uint32_t InputNum = static_cast<uint32_t>(vTargetInfo.InputIndices.size());
    StageInputRefSet.resize(InputNum);
    for (size_t i = 0; i < vTargetInfo.InputIndices.size(); ++i)
    {
        const SAttachmentReference& AttachmentRef = vTargetInfo.InputIndices[i];
        _ASSERTE(AttachmentRef.Index <= getAttachementNum());
        StageInputRefSet[i].attachment = AttachmentRef.Index;
        StageInputRefSet[i].layout = AttachmentRef.Layout;
    }
    SubpassDesc.inputAttachmentCount = InputNum;
    SubpassDesc.pInputAttachments = StageInputRefSet.data();

    m_StageColorRefSetSet.push_back({});
    auto& StageColorRefSet = m_StageColorRefSetSet[m_StageColorRefSetSet.size() - 1];
    uint32_t ColorAttachementNum = static_cast<uint32_t>(vTargetInfo.ColorIndices.size());
    StageColorRefSet.resize(ColorAttachementNum);
    for (size_t i = 0; i < vTargetInfo.ColorIndices.size(); ++i)
    {
        const SAttachmentReference& AttachmentRef = vTargetInfo.ColorIndices[i];
        _ASSERTE(AttachmentRef.Index < m_ColorAttachmentInfoSet.size());
        StageColorRefSet[i].attachment = AttachmentRef.Index;
        StageColorRefSet[i].layout = AttachmentRef.Layout;
    }
    SubpassDesc.colorAttachmentCount = ColorAttachementNum;
    SubpassDesc.pColorAttachments = StageColorRefSet.data();

    _ASSERTE(!vTargetInfo.UseDepth || m_DepthAttachmentInfo.has_value());
    uint32_t DepthAttachmentIndex = m_ColorAttachmentInfoSet.size();
    if (m_DepthAttachmentInfo.has_value())
    {
        m_StageDepthRefSet.push_back({ DepthAttachmentIndex , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }); // TODO: custom layout
        auto& StageDepthRef = m_StageDepthRefSet[m_StageDepthRefSet.size() - 1];
        SubpassDesc.pDepthStencilAttachment = &StageDepthRef;
    }
    else
    {
        SubpassDesc.pDepthStencilAttachment = nullptr;
    }
    return SubpassDesc;
}

void CRenderPassDescriptor::__generateSubpassDescription()
{
    m_StageSubpassDescSet.clear();
    if (m_SubpassReferenceInfoSet.empty()) // did not specify, all attachement are used
    {
        SSubpassReferenceInfo Info;
        for (size_t i = 0; i < m_ColorAttachmentInfoSet.size(); ++i)
            Info.addColorRef(i);
        Info.setUseDepth(m_DepthAttachmentInfo.has_value());
            
        VkSubpassDescription CommonDesc = __generateSubpassDescriptionFromTargetInfo(Info);
        m_StageSubpassDescSet.resize(1, CommonDesc);
    }
    else
    {
        for (const auto& Info : m_SubpassReferenceInfoSet)
        {
            VkSubpassDescription Desc = __generateSubpassDescriptionFromTargetInfo(Info);
            m_StageSubpassDescSet.emplace_back(Desc);
        }
    }
}