#pragma once
#include "PchVulkan.h"
#include "Device.h"
#include <vector>

namespace vk
{
    class CFrameBuffer : public IVulkanHandle<VkFramebuffer>
    {
    public:
        _DEFINE_PTR(CFrameBuffer);

        void create(CDevice::CPtr vDevice, VkRenderPass vRenderPass, const std::vector<VkImageView>& vAttachmentSet, VkExtent2D vExtent);
        void destroy();
    private:
        CDevice::CPtr m_pDevice = nullptr;
    };
}

