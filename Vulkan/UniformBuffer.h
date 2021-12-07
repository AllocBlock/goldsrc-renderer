#pragma once
#include "VulkanHandle.h"
#include "Buffer.h"

namespace vk
{
    class CUniformBuffer : public IVulkanHandle<VkBuffer>
    {
    public:
        void create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkDeviceSize vSize);
        void update(const void* vData);
        void destroy();

    private:
        std::shared_ptr<vk::CBuffer> m_pBuffer = nullptr;
    };
}

