#pragma once
#include "RenderPassPort.h"
#include <vector>

namespace
{
    static const VkClearValue kDefaultClearValueColor = VkClearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
    static const VkClearValue kDefaultClearValueDepth = VkClearValue{ 1.0f, 0 };
}

class CRenderInfoDescriptor
{
public:
    void addColorAttachment(VkImageView vImage, VkFormat vFormat, VkImageLayout vLayout, bool vIsBegin, bool vIsEnd);
    void setDepthAttachment(VkImageView vImage, VkFormat vFormat, VkImageLayout vLayout, bool vIsBegin, bool vIsEnd);
    void addColorAttachment(CPort::Ptr vPort);
    void setDepthAttachment(CPort::Ptr vPort);
    void clear();

    const std::vector<VkFormat>& getColorAttachmentFormats() const;
    bool hasDepthAttachment() const;
    VkFormat getDepthAttachmentFormat() const;

    VkRenderingInfo generateRendererInfo(VkExtent2D renderSize, bool vHasSecondary = false);

   /* bool operator == (const CRenderInfoDescriptor& vDesc) const
    {
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

    bool operator != (const CRenderInfoDescriptor& vDesc) const
    {
        return !(*this == vDesc);
    }*/

    static CRenderInfoDescriptor generateSingleSubpassDesc(CPort::Ptr vColorPort, CPort::Ptr vDepthPort = nullptr);
private:
    //VkSubpassDescription __generateSubpassDescriptionFromTargetInfo(const SSubpassReferenceInfo& vTargetInfo);
    //void __generateSubpassDescription(); // TODO: this just generates a full reference dependency
    //void __generateDependency(); // TODO: this just generates a sequence dependency

    // avoid local point problem
    std::vector<VkRenderingAttachmentInfo> m_AttachmentInfoColors;
    std::vector<VkFormat> m_AttachmentFormatColors;
    std::optional<VkRenderingAttachmentInfo> m_AttachmentInfoDepthStencil; // together with stencil
    std::optional<VkFormat> m_AttachmentFormatDepthStencil;

    //std::vector<std::vector<VkAttachmentReference>> m_StageInputRefSetSet;
    //std::vector<std::vector<VkAttachmentReference>> m_StageColorRefSetSet;
    //std::vector<VkAttachmentReference> m_StageDepthRefSet;
    //std::vector<VkSubpassDescription> m_StageSubpassDescSet;
    //std::vector<VkSubpassDependency> m_StageDepedencySet;
};