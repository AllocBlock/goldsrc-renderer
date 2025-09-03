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
                ~CPhysicalDevice();

        static sptr<CPhysicalDevice> chooseBestDevice(cptr<CInstance> vInstance, cptr<CSurface> vSurface, const std::vector<const char*>& vExtensions);
        void printSupportedExtension() const;
        cptr<CInstance> getInstance() const;
        cptr<CSurface> getSurface() const;
        void release();

        uint32_t findMemoryTypeIndex(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties) const;
        const VkPhysicalDeviceProperties& getProperty() const;
        SQueueFamilyIndices getQueueFamilyInfo() const;
        SSwapChainSupportDetails getSwapChainSupportInfo() const;
        VkFormat chooseSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures) const;
        VkFormatProperties getFormatProperty(VkFormat vFormat) const;

        VkFormat getBestDepthFormat() const;

    private:
        static bool __isDeviceSuitable(VkPhysicalDevice vPhysicalDevice, cptr<CSurface> vSurface, const std::vector<const char*>& vDeviceExtensions);
        static bool __isDeviceExtensionsSupported(VkPhysicalDevice vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions);
        static SQueueFamilyIndices __findQueueFamilies(VkPhysicalDevice vPhysicalDevice, cptr<CSurface> vSurface);
        static SSwapChainSupportDetails __getSwapChainSupport(VkPhysicalDevice vPhysicalDevice, cptr<CSurface> vSurface);

        void __initBestDepthFormat();

        cptr<CInstance> m_pInstance = nullptr;
        cptr<CSurface> m_pSurface = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_DeviceProperty = {};
        VkFormat m_BestDepthFormat = VkFormat::VK_FORMAT_UNDEFINED;
    };
}
