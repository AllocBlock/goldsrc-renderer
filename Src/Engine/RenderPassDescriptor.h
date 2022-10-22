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

// TIPS: before create with generated info, you should not release this descriptor as it contains data pointed in renderpass create info
class CRenderPassDescriptor
{
public:
    void addColorAttachment(CPort::Ptr vPort);
    void setDepthAttachment(CPort::Ptr vPort);
    void addColorAttachment(const SAttachementInfo& vInfo);
    void setDepthAttachment(const SAttachementInfo& vInfo);
    void setSubpassNum(uint32_t vNum);

    bool isValid() const { return m_IsValid; }
    void clearStage(); // you can clear stage data after create renderpass to save memory
    void clear();
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
    void __generateAttachmentDescription();
    void __generateSubpassDescription(); // TODO: this just generates a full reference dependency
    void __generateDependency(); // TODO: this just generates a sequence dependency

    std::vector<SAttachementInfo> m_ColorAttachmentInfoSet;
    std::optional<SAttachementInfo> m_DepthAttachmentInfo = std::nullopt;
    uint32_t m_SubPassNum = 1u;
    bool m_IsValid = true;

    // avoid local point problem
    std::vector<VkAttachmentDescription> m_StageAttachmentDescSet;
    std::vector<VkAttachmentReference> m_StageColorRefSet;
    VkAttachmentReference m_StageDepthRef = {};
    std::vector<VkSubpassDescription> m_StageSubpassDescSet;
    std::vector<VkSubpassDependency> m_StageDepedencySet;
};