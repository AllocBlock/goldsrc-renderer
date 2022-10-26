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

        size_t getAttachmentNum() const { return m_AttachmentNum; }
        VkExtent2D getExtent() const { return m_Extent; }
    private:
        CDevice::CPtr m_pDevice = nullptr;
        size_t m_AttachmentNum = 0;
        VkExtent2D m_Extent = { 0, 0 };
    };
}

