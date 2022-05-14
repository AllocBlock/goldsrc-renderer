#pragma once
#include "PchVulkan.h"
#include "PhysicalDevice.h"
#include "Surface.h"
#include <vector>

namespace vk
{
    class CDevice : public IVulkanHandle<VkDevice>
    {
    public:
        _DEFINE_PTR(CDevice);

        void create(CPhysicalDevice::CPtr vPhysicalDevice, CSurface::CPtr vSurface, const std::vector<const char*>& vExtensionSet, const std::vector<const char*>& vValidationLayerSet);
        void destroy();
        void waitUntilIdle() const;
        uint32_t getGraphicsQueueIndex() const;
        uint32_t getPresentQueueIndex() const;
        VkQueue getGraphicsQueue() const;
        VkQueue getPresentQueue() const;
        VkQueue getQueue(uint32_t vIndex) const;

        VkShaderModule createShaderModule(const std::vector<char>& vShaderCode) const;
        void destroyShaderModule(VkShaderModule vModule) const;
        operator VkDevice() const { return get(); }

    private:
        uint32_t m_GraphicsQueueIndex = 0, m_PresentQueueIndex = 0;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE, m_PresentQueue = VK_NULL_HANDLE;
    };
}
