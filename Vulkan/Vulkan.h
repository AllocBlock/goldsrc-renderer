#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <filesystem>
#include <functional>

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

    struct SImagePack
    {
        VkImage Image = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
        VkImageView ImageView = VK_NULL_HANDLE;

        bool isValid()
        {
            return Image != VK_NULL_HANDLE && Memory != VK_NULL_HANDLE && ImageView != VK_NULL_HANDLE;
        }

        void destroy(VkDevice vDevice)
        {
            if (!isValid()) return;

            vkDestroyImage(vDevice, Image, nullptr);
            vkFreeMemory(vDevice, Memory, nullptr);
            vkDestroyImageView(vDevice, ImageView, nullptr);
            Image = VK_NULL_HANDLE;
            Memory = VK_NULL_HANDLE;
            ImageView = VK_NULL_HANDLE;
        }
    };

    struct SBufferPack
    {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;

        bool isValid()
        {
            return Buffer != VK_NULL_HANDLE && Memory != VK_NULL_HANDLE;
        }

        void destroy(VkDevice vDevice)
        {
            if (!isValid()) return;

            vkDestroyBuffer(vDevice, Buffer, nullptr);
            vkFreeMemory(vDevice, Memory, nullptr);
            Buffer = VK_NULL_HANDLE;
            Memory = VK_NULL_HANDLE;
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

    VkImageView createImageView(VkDevice vDevice, VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags, VkImageViewType vViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D, uint32_t vLayerCount = 1);
    uint32_t findMemoryType(VkPhysicalDevice vPhysicalDevice, uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    void createBuffer(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory);
    void copyBuffer(VkCommandBuffer vCommandBuffer, VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize);
    void createImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkImageCreateInfo vImageInfo, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory);
    void transitionImageLayout(VkCommandBuffer vCommandBuffer, VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount);
    void copyBufferToImage(VkCommandBuffer vCommandBuffer, VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight, uint32_t vLayerCount);
    void stageFillBuffer(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, const void* vData, VkDeviceSize vSize, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory);
    void stageFillImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, const void* vData, VkDeviceSize vSize, VkImageCreateInfo vImageInfo, VkImage& voImage, VkDeviceMemory& voBufferMemory);

    void setSingleTimeBufferFunc(beginSingleTimeBufferFunc_t vBeginFunc, endSingleTimeBufferFunc_t vEndFunc);
    VkCommandBuffer beginSingleTimeBuffer();
    void endSingleTimeBuffer(VkCommandBuffer vCommandBuffer);

    SImagePack createDepthImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage = NULL, VkFormat vFormat = VkFormat::VK_FORMAT_D32_SFLOAT);
}