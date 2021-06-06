#include "Common.h"

#include <set>
#include <string>
#include <fstream>

const float g_ModOffset = 1e-9;

float Common::mod(float vVal, float vMax)
{
    if (vVal < 0)
    {
        int Times = std::ceil(std::abs(vVal) / vMax - g_ModOffset);
        return vVal + Times * vMax;
    }
    else
    {
        int Times = std::floor(vVal / vMax);
        return vVal - Times * vMax;
    }
}

std::vector<char> Common::readFile(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::ate | std::ios::binary);

    if (!File.is_open())
        throw std::runtime_error(u8"¶ÁÈ¡ÎÄ¼þÊ§°Ü£º" + vFilePath.u8string());

    size_t FileSize = static_cast<size_t>(File.tellg());
    std::vector<char> Buffer(FileSize);
    File.seekg(0);
    File.read(Buffer.data(), FileSize);
    File.close();

    return Buffer;
}

VkShaderModule Common::createShaderModule(VkDevice vDevice, const std::vector<char>& vShaderCode)
{
    VkShaderModuleCreateInfo ShaderModuleInfo = {};
    ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleInfo.codeSize = vShaderCode.size();
    ShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vShaderCode.data());

    VkShaderModule ShaderModule;
    ck(vkCreateShaderModule(vDevice, &ShaderModuleInfo, nullptr, &ShaderModule));
    return ShaderModule;
}

VkCommandBuffer Common::beginSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool)
{
    VkCommandBufferAllocateInfo CommandBufferAllocInfo = {};
    CommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocInfo.commandPool = vCommandPool;
    CommandBufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer CommandBuffer;
    ck(vkAllocateCommandBuffers(vDevice, &CommandBufferAllocInfo, &CommandBuffer));

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    return CommandBuffer;
}

void Common::endSingleTimeCommands(VkDevice vDevice, VkCommandPool vCommandPool, VkQueue vGraphicQueue, VkCommandBuffer vCommandBuffer)
{
    ck(vkEndCommandBuffer(vCommandBuffer));

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &vCommandBuffer;

    ck(vkQueueSubmit(vGraphicQueue, 1, &SubmitInfo, VK_NULL_HANDLE));
    ck(vkQueueWaitIdle(vGraphicQueue));

    vkFreeCommandBuffers(vDevice, vCommandPool, 1, &vCommandBuffer);
}

Common::SQueueFamilyIndices Common::findQueueFamilies(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface)
{
    uint32_t NumQueueFamily = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &NumQueueFamily, nullptr);
    std::vector<VkQueueFamilyProperties> QueueFamilies(NumQueueFamily);
    vkGetPhysicalDeviceQueueFamilyProperties(vPhysicalDevice, &NumQueueFamily, QueueFamilies.data());

    SQueueFamilyIndices Indices;
    for (size_t i = 0; i < NumQueueFamily; ++i)
    {
        if (QueueFamilies[i].queueCount > 0 &&
            QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            Indices.GraphicsFamilyIndex = i;
        }
        VkBool32 PresentSupport = false;
        ck(vkGetPhysicalDeviceSurfaceSupportKHR(vPhysicalDevice, i, vSurface, &PresentSupport));

        if (QueueFamilies[i].queueCount > 0 && PresentSupport)
        {
            Indices.PresentFamilyIndex = i;
        }
        if (Indices.isComplete())
        {
            break;
        }
    }
    return Indices;
}

bool Common::checkValidationLayerSupport(const std::vector<const char*>& vValidationLayers)
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

bool Common::checkDeviceExtensionSupport(const VkPhysicalDevice& vPhysicalDevice, const std::vector<const char*>& vDeviceExtensions)
{
    uint32_t NumExtensions;
    ck(vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &NumExtensions, nullptr));
    std::vector<VkExtensionProperties> AvailableExtensions(NumExtensions);
    ck(vkEnumerateDeviceExtensionProperties(vPhysicalDevice, nullptr, &NumExtensions, AvailableExtensions.data()));

    std::set<std::string> RequiredExtensions(vDeviceExtensions.begin(), vDeviceExtensions.end());

    for (const auto& Extension : AvailableExtensions) {
        RequiredExtensions.erase(Extension.extensionName);
    }

    return RequiredExtensions.empty();
}

bool Common::isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface, const std::vector<const char*>& vDeviceExtensions)
{
    SQueueFamilyIndices QueueIndices = Common::findQueueFamilies(vPhysicalDevice, vSurface);
    if (!QueueIndices.isComplete()) return false;

    bool ExtensionsSupported = Common::checkDeviceExtensionSupport(vPhysicalDevice, vDeviceExtensions);
    if (!ExtensionsSupported) return false;

    bool SwapChainAdequate = false;  
    Common::SSwapChainSupportDetails SwapChainSupport = Common::getSwapChainSupport(vPhysicalDevice, vSurface);
    SwapChainAdequate = !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();
    if (!SwapChainAdequate) return false;

    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(vPhysicalDevice, &SupportedFeatures);
    if (!SupportedFeatures.samplerAnisotropy) return false;

    return true;
}

Common::SSwapChainSupportDetails Common::getSwapChainSupport(const VkPhysicalDevice& vPhysicalDevice, const VkSurfaceKHR& vSurface)
{
    Common::SSwapChainSupportDetails Details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vPhysicalDevice, vSurface, &Details.Capabilities);

    uint32_t NumFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, vSurface, &NumFormats, nullptr);
    if (NumFormats != 0)
    {
        Details.Formats.resize(NumFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vPhysicalDevice, vSurface, &NumFormats, Details.Formats.data());
    }

    uint32_t NumPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, vSurface, &NumPresentModes, nullptr);
    if (NumPresentModes != 0)
    {
        Details.PresentModes.resize(NumPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vPhysicalDevice, vSurface, &NumPresentModes, Details.PresentModes.data());
    }

    return Details;
}

VkImageView Common::createImageView(VkDevice vDevice, VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags, VkImageViewType vViewType, uint32_t vLayerCount)
{
    VkImageViewCreateInfo ImageViewInfo = {};
    ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewInfo.image = vImage;
    ImageViewInfo.viewType = vViewType;
    ImageViewInfo.format = vFormat;
    ImageViewInfo.subresourceRange.aspectMask = vAspectFlags;
    ImageViewInfo.subresourceRange.baseMipLevel = 0;
    ImageViewInfo.subresourceRange.levelCount = 1;
    ImageViewInfo.subresourceRange.baseArrayLayer = 0;
    ImageViewInfo.subresourceRange.layerCount = vLayerCount;

    VkImageView ImageView;
    ck(vkCreateImageView(vDevice, &ImageViewInfo, nullptr, &ImageView));

    return ImageView;
}