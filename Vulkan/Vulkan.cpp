#include "Vulkan.h"

#include <string>
#include <fstream>

// DEBUGÊä³ö
#ifdef _DEBUG
#include <iostream>
#include <iomanip>
#endif

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

vk::beginSingleTimeBufferFunc_t g_BeginFunc = nullptr;
vk::endSingleTimeBufferFunc_t g_EndFunc = nullptr;

void vk::setSingleTimeBufferFunc(vk::beginSingleTimeBufferFunc_t vBeginFunc, vk::endSingleTimeBufferFunc_t vEndFunc)
{
    g_BeginFunc = vBeginFunc;
    g_EndFunc = vEndFunc;
}

void vk::removeSingleTimeBufferFunc()
{
    g_BeginFunc = nullptr;
    g_EndFunc = nullptr;
}

VkCommandBuffer vk::beginSingleTimeBuffer()
{
    _ASSERTE(g_BeginFunc != nullptr);
    return g_BeginFunc();
}

void vk::endSingleTimeBuffer(VkCommandBuffer vCommandBuffer)
{
    _ASSERTE(g_EndFunc != nullptr);
    g_EndFunc(vCommandBuffer);
}