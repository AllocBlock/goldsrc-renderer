#pragma once
#include "RenderPassPort.h"
#include "Image.h"
#include <vector>

class CRenderInfoDescriptor
{
public:
    void addColorAttachment(VkImageView vImage, VkFormat vFormat, bool clear);
    void setDepthAttachment(VkImageView vImage, VkFormat vFormat, bool clear);
    void addColorAttachment(vk::CImage::Ptr vImage, bool clear);
    void setDepthAttachment(vk::CImage::Ptr vImage, bool clear);
    void addColorAttachment(CPort::Ptr vPort);
    void setDepthAttachment(CPort::Ptr vPort);
    void clear();

    const std::vector<VkFormat>& getColorAttachmentFormats() const;
    bool hasDepthAttachment() const;
    VkFormat getDepthAttachmentFormat() const;

    VkRenderingInfo generateRendererInfo(VkExtent2D renderSize, bool vHasSecondary = false);

    static CRenderInfoDescriptor generateSingleSubpassDesc(CPort::Ptr vColorPort, CPort::Ptr vDepthPort = nullptr);
private:
    std::vector<VkRenderingAttachmentInfo> m_AttachmentInfoColors;
    std::vector<VkFormat> m_AttachmentFormatColors;
    std::optional<VkRenderingAttachmentInfo> m_AttachmentInfoDepthStencil; // together with stencil
    std::optional<VkFormat> m_AttachmentFormatDepthStencil;
};