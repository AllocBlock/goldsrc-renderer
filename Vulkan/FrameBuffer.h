#pragma once
#include "VulkanHandle.h"
#include <vector>

namespace vk
{
    class CFrameBuffer : public IVulkanHandle<VkFramebuffer>
    {
    public:
        void create(VkDevice vDevice, VkRenderPass vRenderPass, const std::vector<VkImageView>& vAttachmentSet, VkExtent2D vExtent);
        void destroy();
    private:
        VkDevice m_Device = VK_NULL_HANDLE;
    };
}

