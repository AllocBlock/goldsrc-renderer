#include "FrameBuffer.h"
#include "Vulkan.h"

using namespace vk;

void CFrameBuffer::create(VkDevice vDevice, VkRenderPass vRenderPass, const std::vector<VkImageView>& vAttachmentSet, VkExtent2D vExtent)
{
    destroy();

    m_Device = vDevice;

    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = vRenderPass;
    FramebufferInfo.attachmentCount = static_cast<uint32_t>(vAttachmentSet.size());
    FramebufferInfo.pAttachments = vAttachmentSet.data();
    FramebufferInfo.width = vExtent.width;
    FramebufferInfo.height = vExtent.height;
    FramebufferInfo.layers = 1;

    Vulkan::checkError(vkCreateFramebuffer(vDevice, &FramebufferInfo, nullptr, &m_Handle));
}

void CFrameBuffer::destroy()
{
    if (m_Handle) vkDestroyFramebuffer(m_Device, m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;

    m_Device = VK_NULL_HANDLE;
}
