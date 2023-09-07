#pragma once
#include "PchVulkan.h"
#include <vector>
#include <optional>
#include <filesystem>

bool operator == (const VkExtent2D& v1, const VkExtent2D& v2);
bool operator != (const VkExtent2D& v1, const VkExtent2D& v2);

namespace vk
{
    extern const VkExtent2D& ZeroExtent;

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
    
    inline void checkError(VkResult vResult) { if (vResult) throw std::runtime_error(u8"vulkan·µ»Ø´íÎó"); }
    bool isValidationLayersSupported(const std::vector<const char*>& vValidationLayers);
    
    float calcAspect(const VkExtent2D& vExtent);
}
