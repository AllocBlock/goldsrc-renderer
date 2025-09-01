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

        void create(CPhysicalDevice::CPtr vPhysicalDevice, const std::vector<const char*>& vExtensionSet, const std::vector<const char*>& vValidationLayerSet);
        void destroy();
        void waitUntilIdle() const;
        CPhysicalDevice::CPtr getPhysicalDevice() const;
        uint32_t getGraphicsQueueIndex() const;
        uint32_t getPresentQueueIndex() const;
        VkQueue getGraphicsQueue() const;
        VkQueue getPresentQueue() const;
        VkQueue getQueue(uint32_t vIndex) const;

        VkShaderModule createShaderModule(const std::vector<uint8_t>& vShaderCode) const;
        void destroyShaderModule(VkShaderModule vModule) const;
        VkSemaphore createSemaphore() const;
        void destroySemaphore(VkSemaphore vSemaphore) const;

        void setObjectDebugName(VkObjectType type, uint64_t handle, const std::string& vName) const;

    private:
        CPhysicalDevice::CPtr m_pPhysicalDevice = nullptr;
        uint32_t m_GraphicsQueueIndex = 0, m_PresentQueueIndex = 0;
        VkQueue m_GraphicsQueue = VK_NULL_HANDLE, m_PresentQueue = VK_NULL_HANDLE;
        PFN_vkSetDebugUtilsObjectNameEXT m_vkSetDebugUtilsObjectNameEXTFunc = nullptr;
    };
}
