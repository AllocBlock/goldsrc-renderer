#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <filesystem>

#define ck(E) do{if (E) throw std::runtime_error(u8"vulkan·µ»Ø´íÎó");} while(0)

namespace Common
{
    struct SVkImagePack
    {
        VkImage Image = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
        VkImageView ImageView = VK_NULL_HANDLE;

        bool isValid()
        {
            return Image != VK_NULL_HANDLE && Memory != VK_NULL_HANDLE && ImageView != VK_NULL_HANDLE;
        }

        void destory(VkDevice vDevice)
        {
            vkDestroyImage(vDevice, Image, nullptr);
            vkFreeMemory(vDevice, Memory, nullptr);
            vkDestroyImageView(vDevice, ImageView, nullptr);
            Image = VK_NULL_HANDLE;
            Memory = VK_NULL_HANDLE;
            ImageView = VK_NULL_HANDLE;
        }
    };

    struct SVkBufferPack
    {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;

        bool isValid()
        {
            return Buffer != VK_NULL_HANDLE && Memory != VK_NULL_HANDLE;
        }

        void destory(VkDevice vDevice)
        {
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

    float mod(float vVal, float vMax);

    std::vector<char> readFileAsChar(std::filesystem::path vFilePath);
    VkShaderModule createShaderModule(VkDevice vDevice, const std::vector<char>& vShaderCode);
    SQueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);
    SSwapChainSupportDetails getSwapChainSupport(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);
    bool checkValidationLayerSupport(const std::vector<const char*>& vValidationLayers);
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions);
    bool isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions);

    VkImageView createImageView(VkDevice vDevice, VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags, VkImageViewType vViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D, uint32_t vLayerCount = 1);
}
