#include "PchVulkan.h"
#include "FrameBuffer.h"
#include "Vulkan.h"

using namespace vk;

void CFrameBuffer::create(CDevice::CPtr vDevice, VkRenderPass vRenderPass, const std::vector<VkImageView>& vAttachmentSet, VkExtent2D vExtent)
{
    destroy();

    m_pDevice = vDevice;
    m_AttachmentNum = vAttachmentSet.size();
    m_Extent = vExtent;

    VkFramebufferCreateInfo FramebufferInfo = {};
    FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FramebufferInfo.renderPass = vRenderPass;
    FramebufferInfo.attachmentCount = static_cast<uint32_t>(vAttachmentSet.size());
    FramebufferInfo.pAttachments = vAttachmentSet.data();
    FramebufferInfo.width = vExtent.width;
    FramebufferInfo.height = vExtent.height;
    FramebufferInfo.layers = 1;

    vk::checkError(vkCreateFramebuffer(*vDevice, &FramebufferInfo, nullptr, _getPtr()));
#ifdef _DEBUG
    static int Count = 0;
    std::cout << "create framebuffer [" << Count << "] = 0x" << std::setbase(16) << (uint64_t)(get())  << " by 0x" << (uint64_t)(this) << std::setbase(10) << std::endl;
    Count++;
#endif
}

void CFrameBuffer::destroy()
{
    if (get()) vkDestroyFramebuffer(*m_pDevice, get(), nullptr);
    _setNull();

    m_pDevice = nullptr;
}
