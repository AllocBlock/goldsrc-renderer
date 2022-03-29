#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <limits>
#include <filesystem>
#include <functional>
#include "Image.h"

namespace Vulkan
{
    struct SVulkanAppInfo
    {
        SVulkanAppInfo() { clear(); }
        VkInstance Instance;
        VkPhysicalDevice PhysicalDevice;
        VkDevice Device;
        uint32_t GraphicsQueueIndex;
        VkQueue GraphicsQueue;
        VkExtent2D Extent;
        VkFormat ImageFormat;
        std::vector<VkImageView> TargetImageViewSet;

        void clear()
        {
            Instance = VK_NULL_HANDLE;
            PhysicalDevice = VK_NULL_HANDLE;
            Device = VK_NULL_HANDLE;
            GraphicsQueueIndex = std::numeric_limits<uint32_t>::max();
            GraphicsQueue = VK_NULL_HANDLE;
            Extent = { 0, 0 };
            ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
            TargetImageViewSet.clear();
        }
    };

    struct SQueueFamilyIndices
    {
        std::optional<uint32_t> GraphicsFamilyIndex;
        std::optional<uint32_t> PresentFamilyIndex;

        bool isComplete()
        {
            return GraphicsFamilyIndex.has_value() && PresentFamilyIndex.has_value();
        }
    };

    struct SSwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    using beginSingleTimeBufferFunc_t = std::function<VkCommandBuffer()>;
    using endSingleTimeBufferFunc_t = std::function<void(VkCommandBuffer)>;

    inline void checkError(VkResult vResult) { if (vResult) throw std::runtime_error(u8"vulkan·µ»Ø´íÎó"); }
    VkShaderModule createShaderModule(VkDevice vDevice, const std::vector<char>& vShaderCode);
    SQueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);
    SSwapChainSupportDetails getSwapChainSupport(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);
    bool checkValidationLayerSupport(const std::vector<const char*>& vValidationLayers);
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions);
    bool isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions);

    uint32_t findMemoryType(VkPhysicalDevice vPhysicalDevice, uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);

    void setSingleTimeBufferFunc(beginSingleTimeBufferFunc_t vBeginFunc, endSingleTimeBufferFunc_t vEndFunc);
    void removeSingleTimeBufferFunc();
    VkCommandBuffer beginSingleTimeBuffer();
    void endSingleTimeBuffer(VkCommandBuffer vCommandBuffer);

    vk::CImage::Ptr createDepthImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage = NULL, VkFormat vFormat = VkFormat::VK_FORMAT_D32_SFLOAT);
}