#pragma once
#include "VulkanHandle.h"
#include "PhysicalDevice.h"
#include "Device.h"

namespace vk
{
    class CBuffer : public IVulkanHandle<VkBuffer>
    {
    public:
        _DEFINE_PTR(CBuffer);

        void create(CPhysicalDevice::CPtr vPhysicalDevice, CDevice::CPtr vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties);
        void destroy();
        bool isValid();
        void copyFrom(VkCommandBuffer vCommandBuffer, VkBuffer vSrcBuffer, VkDeviceSize vSize);
        void copyToHost(VkDeviceSize vSize, void* pPtr);
        void fill(const void* vData, VkDeviceSize vSize);
        void stageFill(const void* vData, VkDeviceSize vSize);
        VkDeviceSize getSize();

    private:
        CPhysicalDevice::CPtr m_pPhysicalDevice = nullptr;
        CDevice::CPtr m_pDevice = nullptr;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkDeviceSize m_Size = 0;
    };
}
