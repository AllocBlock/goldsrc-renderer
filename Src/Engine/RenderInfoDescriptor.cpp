#include "RenderInfoDescriptor.h"

namespace
{
    static const VkClearValue kDefaultClearValueColor = VkClearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
    static const VkClearValue kDefaultClearValueDepth = VkClearValue{ 1.0f, 0 };
}

void CRenderInfoDescriptor::addColorAttachment(VkImageView vImage, VkFormat vFormat, bool clear)
{
    VkAttachmentLoadOp LoadOp = clear ? VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR : VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

    VkRenderingAttachmentInfo Info = {};
    Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    Info.imageView = vImage;
    Info.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    Info.loadOp = LoadOp;
    Info.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    Info.clearValue = kDefaultClearValueColor;

    m_AttachmentInfoColors.emplace_back(Info);
    m_AttachmentFormatColors.emplace_back(vFormat);
}

void CRenderInfoDescriptor::setDepthAttachment(VkImageView vImage, VkFormat vFormat, bool clear)
{
    VkAttachmentLoadOp LoadOp = clear ? VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR : VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_LOAD;

    VkRenderingAttachmentInfo Info = {};
    Info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    Info.imageView = vImage;
    Info.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    Info.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
    Info.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
    Info.clearValue = kDefaultClearValueDepth;

    m_AttachmentInfoDepthStencil = Info;
    m_AttachmentFormatDepthStencil = vFormat;
}

void CRenderInfoDescriptor::addColorAttachment(vk::CImage::Ptr vImage, bool clear)
{
    addColorAttachment(*vImage, vImage->getFormat(), clear);
}

void CRenderInfoDescriptor::setDepthAttachment(vk::CImage::Ptr vImage, bool clear)
{
    setDepthAttachment(*vImage, vImage->getFormat(), clear);
}

void CRenderInfoDescriptor::addColorAttachment(CPort::Ptr vPort)
{
    _ASSERT(vPort->getLayout() == VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    addColorAttachment(vPort->getImageV(), vPort->isRoot());
}

void CRenderInfoDescriptor::setDepthAttachment(CPort::Ptr vPort)
{
    _ASSERT(vPort->getLayout() == VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    setDepthAttachment(vPort->getImageV(), vPort->isRoot());
}

void CRenderInfoDescriptor::clear()
{
    m_AttachmentInfoColors.clear();
    m_AttachmentInfoDepthStencil = std::nullopt;
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
