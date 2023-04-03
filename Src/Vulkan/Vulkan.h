#pragma once

#include "PchVulkan.h"
#include <vector>
#include <optional>
#include <filesystem>
#include <functional>

bool operator == (const VkExtent2D& v1, const VkExtent2D& v2);
bool operator != (const VkExtent2D& v1, const VkExtent2D& v2);

class CCommandBuffer; // forward declaration

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

    using beginSingleTimeBufferFunc_t = std::function<ptr<CCommandBuffer>()>;
    using endSingleTimeBufferFunc_t = std::function<void(ptr<CCommandBuffer>&)>;

    inline void checkError(VkResult vResult) { if (vResult) throw std::runtime_error(u8"vulkan·µ»Ø´íÎó"); }
    bool isValidationLayersSupported(const std::vector<const char*>& vValidationLayers);

    void setSingleTimeBufferFunc(beginSingleTimeBufferFunc_t vBeginFunc, endSingleTimeBufferFunc_t vEndFunc);
    void removeSingleTimeBufferFunc();
    ptr<CCommandBuffer> beginSingleTimeBuffer();
    void endSingleTimeBuffer(ptr<CCommandBuffer>& voCommandBuffer);
}