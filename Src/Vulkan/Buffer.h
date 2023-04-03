#pragma once
#include "PchVulkan.h"
#include "Device.h"
#include "CommandBuffer.h"

namespace vk
{
    class CBuffer : public IVulkanHandle<VkBuffer>
    {
    public:
        _DEFINE_PTR(CBuffer);

        void create(CDevice::CPtr vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties);
        void destroy();
        bool isValid() const override;
        void copyFrom(CCommandBuffer::Ptr vCommandBuffer, VkBuffer vSrcBuffer, VkDeviceSize vSize);
        void copyToHost(VkDeviceSize vSize, void* pPtr);
        void fill(const void* vData, VkDeviceSize vSize);
        void stageFill(const void* vData, VkDeviceSize vSize);
        VkDeviceSize getSize() const;

    private:
        CDevice::CPtr m_pDevice = nullptr;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkDeviceSize m_Size = 0;
    };
}
