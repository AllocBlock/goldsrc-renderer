#pragma once
#include "VulkanHandle.h"
#include <vector>

namespace vk
{
    class CDevice : public IVulkanHandle<VkDevice>
    {
    public:
        void create(VkPhysicalDevice vPhysicalDevice, VkSurfaceKHR vSurface, const std::vector<const char*>& vExtensionSet, const std::vector<const char*>& vValidationLayerSet);
        void destroy();
        void waitUntilIdle();
        uint32_t getGraphicsQueueIndex();
        uint32_t getPresentQueueIndex();
        VkQueue getGraphicsQueue();
        VkQueue getPresentQueue();

    private:
        uint32_t m_GraphicsQueueIndex = 0, m_PresentQueueIndex = 0;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE, m_PresentQueue = VK_NULL_HANDLE;
    };
}
