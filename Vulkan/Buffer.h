#pragma once
#include "VulkanHandle.h"

namespace vk
{
    class CBuffer : public IVulkanHandle<VkBuffer>
    {
    public:
        void create(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties);
        void destroy();
        bool isValid();
        void copyFrom(VkCommandBuffer vCommandBuffer, VkBuffer vSrcBuffer, VkDeviceSize vSize);
        void copyToHost(VkDeviceSize vSize, void* pPtr);
        void fill(const void* vData, VkDeviceSize vSize);
        void stageFill(const void* vData, VkDeviceSize vSize);
        VkDeviceSize getSize();

    private:
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkDeviceSize m_Size = 0;
    };
}
