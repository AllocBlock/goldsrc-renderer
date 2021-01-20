#pragma once

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <stdexcept>

#define ck(E) do{if (E) throw std::runtime_error(u8"vulkan·µ»Ø´íÎó");} while(0)

namespace Common
{
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

    VkCommandBuffer beginSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool);
    void endSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool, VkQueue vGraphicQueue, VkCommandBuffer vCommandBuffer);
    SQueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);
    SSwapChainSupportDetails getSwapChainSupport(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface);
    bool checkValidationLayerSupport(const std::vector<const char*>& vValidationLayers);
    bool checkDeviceExtensionSupport(const VkPhysicalDevice& vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions);
    bool isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions);

    VkImageView createImageView(VkDevice vDevice, VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags);
}
