#include "PchVulkan.h"
#include "Vulkan.h"

bool operator == (const VkExtent2D& v1, const VkExtent2D& v2)
{
    return v1.width == v2.width && v1.height == v2.height;
}

bool operator != (const VkExtent2D& v1, const VkExtent2D& v2)
{
    return !(v1 == v2);
}

const VkExtent2D& vk::ZeroExtent = { 0, 0 };

bool vk::isValidationLayersSupported(const std::vector<const char*>& vValidationLayers)
{
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);
    std::vector<VkLayerProperties> AvailableLayers(LayerCount);
    vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.data());

    for (const char* RequiredLayerName : vValidationLayers) {
        bool LayerFound = false;
        for (const auto& Layer : AvailableLayers) {
            if (strcmp(RequiredLayerName, Layer.layerName) == 0) {
                LayerFound = true;
                break;
            }
        }
        if (!LayerFound) {
            return false;
        }
    }
    return true;
}

float vk::calcAspect(const VkExtent2D& vExtent)
{
    _ASSERTE(vExtent.width > 0 && vExtent.height > 0);
    return float(vExtent.width) / vExtent.height;
}
