#pragma once
#include "RenderPassPort.h"
#include <vector>

struct SAttachementInfo
{
    VkFormat Format = VkFormat::VK_FORMAT_UNDEFINED;
    VkImageLayout InitLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout FinalLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
    bool IsBegin = false;
    bool IsEnd = false;

    bool operator == (const SAttachementInfo& vInfo) const
    {
        return Format == vInfo.Format && InitLayout == vInfo.InitLayout && FinalLayout == vInfo.FinalLayout;
    }

    bool operator != (const SAttachementInfo& vInfo) const
    {
        return !(*this == vInfo);
    }
};

struct SAttachmentReference
{
    uint32_t Index;
    VkImageLayout Layout;
};

struct SSubpassReferenceInfo
{
    std::vector<SAttachmentReference> ColorIndices; // color render target indices
    bool UseDepth = false; // if use depth render target
    std::vector<SAttachmentReference> InputIndices; // indices of color/depth attachment as input
    std::vector<uint32_t> DependentPassIndices;

    SSubpassReferenceInfo& addColorRef(uint32_t vIndex, VkImageLayout vLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        ColorIndices.push_back({ vIndex, vLayout });
        return *this;
    };
    SSubpassReferenceInfo& setUseDepth(bool vUseDepth)
    {
        UseDepth = vUseDepth;
        return *this;
    };
    SSubpassReferenceInfo& addInputRef(uint32_t vIndex, VkImageLayout vLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        InputIndices.push_back({ vIndex, vLayout });
        return *this;
    };
    SSubpassReferenceInfo& addDependentPass(uint32_t vIndex)
    {
        DependentPassIndices.push_back(vIndex);
        return *this;
    };
};

// TIPS: before create with generated info, you should not release this descriptor as it contains data pointed in renderpass create info
class CRenderPassDescriptor
{
public:
    CRenderPassDescriptor() { m_IsValid = true; }
    CRenderPassDescriptor(bool vIsDefaultValid) { m_IsValid = vIsDefaultValid; }

    void addColorAttachment(CPort::Ptr vPort);
    void setDepthAttachment(CPort::Ptr vPort);
    void addColorAttachment(const SAttachementInfo& vInfo);
    void setDepthAttachment(const SAttachementInfo& vInfo);
    void addSubpass(const SSubpassReferenceInfo& vSubpassInfo);

    bool isValid() const { return m_IsValid; }
    void clearStage(); // you can clear stage data after create renderpass to save memory
    void clear();
    uint32_t getAttachementNum() const;
    VkRenderPassCreateInfo generateInfo();

    bool operator == (const CRenderPassDescriptor& vDesc) const
    {
        if (isValid() != vDesc.isValid()) return false;
        if (m_ColorAttachmentInfoSet.size() != vDesc.m_ColorAttachmentInfoSet.size()) return false;
        if (m_DepthAttachmentInfo.has_value() != vDesc.m_DepthAttachmentInfo.has_value()) return false;
        size_t ColorNum = m_ColorAttachmentInfoSet.size();
        for (size_t i = 0; i < ColorNum; ++i)
        {
            if (m_ColorAttachmentInfoSet[i] != vDesc.m_ColorAttachmentInfoSet[i]) return false;
        }

        if (m_DepthAttachmentInfo.has_value())
            if (m_DepthAttachmentInfo.value() != vDesc.m_DepthAttachmentInfo.value()) return false;
        return true;
    }

    bool operator != (const CRenderPassDescriptor& vDesc) const
    {
        return !(*this == vDesc);
    }

    static CRenderPassDescriptor generateSingleSubpassDesc(CPort::Ptr vColorPort, CPort::Ptr vDepthPort = nullptr);
private:

    SAttachementInfo __getAttachmentInfo(CPort::Ptr vPort, bool vIsDepth);

    static VkAttachmentDescription __createAttachmentDescription(const SAttachementInfo& vInfo, bool vIsDepth);
    VkSubpassDescription __generateSubpassDescriptionFromTargetInfo(const SSubpassReferenceInfo& vTargetInfo);
    void __generateAttachmentDescription();
    void __generateSubpassDescription(); // TODO: this just generates a full reference dependency
    void __generateDependency(); // TODO: this just generates a sequence dependency
    
    std::vector<SAttachementInfo> m_ColorAttachmentInfoSet;
    std::optional<SAttachementInfo> m_DepthAttachmentInfo = std::nullopt;
    std::vector<SSubpassReferenceInfo> m_SubpassReferenceInfoSet;
    bool m_IsValid = false;

    // avoid local point problem
    std::vector<VkAttachmentDescription> m_StageAttachmentDescSet;
    std::vector<std::vector<VkAttachmentReference>> m_StageInputRefSetSet;
    std::vector<std::vector<VkAttachmentReference>> m_StageColorRefSetSet;
    std::vector<VkAttachmentReference> m_StageDepthRefSet;
    std::vector<VkSubpassDescription> m_StageSubpassDescSet;
    std::vector<VkSubpassDependency> m_StageDepedencySet;
};