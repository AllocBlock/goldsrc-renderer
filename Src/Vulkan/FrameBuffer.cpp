#include "PchVulkan.h"
#include "FrameBuffer.h"
#include "Vulkan.h"

using namespace vk;

void CFrameBuffer::create(CDevice::CPtr vDevice, VkRenderPass vRenderPass, const std::vector<VkImageView>& vAttachmentSet, VkExtent2D vExtent)
{
    destroy();

    m_pDevice = vDevice;
    m_AttachmentNum = vAttachmentSet.size();

    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = vRenderPass;
    FramebufferInfo.attachmentCount = static_cast<uint32_t>(vAttachmentSet.size());
    FramebufferInfo.pAttachments = vAttachmentSet.data();
    FramebufferInfo.width = vExtent.width;
    FramebufferInfo.height = vExtent.height;
    FramebufferInfo.layers = 1;

    vk::checkError(vkCreateFramebuffer(*vDevice, &FramebufferInfo, nullptr, _getPtr()));
}

void CFrameBuffer::destroy()
{
    if (get()) vkDestroyFramebuffer(*m_pDevice, get(), nullptr);
    _setNull();

    m_pDevice = VK_NULL_HANDLE;
}
