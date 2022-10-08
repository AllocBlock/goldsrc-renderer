#pragma once
#include "PchVulkan.h"
#include "Instance.h"
#include "Surface.h"
#include "Vulkan.h"
#include <vector>

namespace vk
{
    class CPhysicalDevice : public IVulkanHandle<VkPhysicalDevice>
    {
    public:
        _DEFINE_PTR(CPhysicalDevice);

        ~CPhysicalDevice();

        static CPhysicalDevice::Ptr chooseBestDevice(CInstance::CPtr vInstance, CSurface::CPtr vSurface, const std::vector<const char*>& vExtensions);
        void printSupportedExtension() const;
        CInstance::CPtr getInstance() const;
        CSurface::CPtr getSurface() const;
        void release();

        uint32_t findMemoryTypeIndex(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties) const;
        const VkPhysicalDeviceProperties& getProperty() const;
        SQueueFamilyIndices getQueueFamilyInfo() const;
        SSwapChainSupportDetails getSwapChainSupportInfo() const;
        VkFormat chooseSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures) const;
        VkFormatProperties getFormatProperty(VkFormat vFormat) const;

        VkFormat getBestDepthFormat() const;

    private:
        static bool __isDeviceSuitable(VkPhysicalDevice vPhysicalDevice, CSurface::CPtr vSurface, const std::vector<const char*>& vDeviceExtensions);
        static bool __isDeviceExtensionsSupported(VkPhysicalDevice vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions);
        static SQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vPhysicalDevice, CSurface::CPtr vSurface);
        static SSwapChainSupportDetails __getSwapChainSupport(VkPhysicalDevice vPhysicalDevice, CSurface::CPtr vSurface);

        CInstance::CPtr m_pInstance = nullptr;
        CSurface::CPtr m_pSurface = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_DeviceProperty = {};
    };
}
