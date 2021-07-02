#include "Common.h"

#include <set>
#include <string>
#include <fstream>
#ifdef _DEBUG
#include <iostream>
#include <iomanip>
#endif

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

std::vector<char> Common::readFileAsChar(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::ate | std::ios::binary);

    if (!File.is_open())
        throw std::runtime_error(u8"读取文件失败：" + vFilePath.u8string());

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

#ifdef _DEBUG
    std::cout << "create image view 0x" << std::setbase(16) << (uint64_t)(ImageView) << std::setbase(10) << std::endl;
#endif
    return ImageView;
}

uint32_t Common::findMemoryType(VkPhysicalDevice vPhysicalDevice, uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties)
{
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(vPhysicalDevice, &MemProperties);
    for (uint32_t i = 0; i < MemProperties.memoryTypeCount; ++i)
    {
        if (vTypeFilter & (1 << i) &&
            (MemProperties.memoryTypes[i].propertyFlags & vProperties))
        {
            return i;
        }
    }

    throw std::runtime_error(u8"未找到合适的存储类别");
}

void Common::createBuffer(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory)
{
    _ASSERTE(vSize > 0);
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = vSize;
    BufferInfo.usage = vUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ck(vkCreateBuffer(vDevice, &BufferInfo, nullptr, &voBuffer));

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(vDevice, voBuffer, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = Common::findMemoryType(vPhysicalDevice, MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(vDevice, &AllocInfo, nullptr, &voBufferMemory));

    ck(vkBindBufferMemory(vDevice, voBuffer, voBufferMemory, 0));
}

void Common::copyBuffer(VkCommandBuffer vCommandBuffer, VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize)
{
    VkBufferCopy CopyRegion = {};
    CopyRegion.size = vSize;
    vkCmdCopyBuffer(vCommandBuffer, vSrcBuffer, vDstBuffer, 1, &CopyRegion);
}

void Common::createImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, VkImageCreateInfo vImageInfo, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory)
{
    ck(vkCreateImage(vDevice, &vImageInfo, nullptr, &voImage));

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(vDevice, voImage, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = Common::findMemoryType(vPhysicalDevice, MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(vDevice, &AllocInfo, nullptr, &voImageMemory));

    ck(vkBindImageMemory(vDevice, voImage, voImageMemory, 0));

#ifdef _DEBUG
    std::cout << "create image 0x" << std::setbase(16) << (uint64_t)(voImage) << std::setbase(10) << std::endl;
    std::cout << "create memory 0x" << std::setbase(16) << (uint64_t)(voImageMemory) << std::setbase(10) << std::endl;
#endif
}

void Common::transitionImageLayout(VkCommandBuffer vCommandBuffer, VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount) {
    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = vOldLayout;
    Barrier.newLayout = vNewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = vImage;

    if (vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        bool hasStencilComponent = (vFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vFormat == VK_FORMAT_D24_UNORM_S8_UINT);
        if (hasStencilComponent)
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = vLayerCount;

    VkPipelineStageFlags SrcStage;
    VkPipelineStageFlags DestStage;

    if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED
        && vNewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        throw std::runtime_error(u8"不支持该布局转换");
    }

    vkCmdPipelineBarrier(
        vCommandBuffer,
        SrcStage, DestStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &Barrier
    );
}

void Common::copyBufferToImage(VkCommandBuffer vCommandBuffer, VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight, uint32_t vLayerCount)
{
    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = vLayerCount;

    Region.imageOffset = { 0, 0, 0 };
    Region.imageExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight), 1 };

    vkCmdCopyBufferToImage(vCommandBuffer, vBuffer, vImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

}

void Common::stageFillBuffer(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, const void* vData, VkDeviceSize vSize, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory)
{
    Common::SBufferPack StageBufferPack;
    Common::createBuffer(vPhysicalDevice, vDevice, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StageBufferPack.Buffer, StageBufferPack.Memory);

    void* pDevData;
    ck(vkMapMemory(vDevice, StageBufferPack.Memory, 0, vSize, 0, &pDevData));
    memcpy(reinterpret_cast<char*>(pDevData), vData, vSize);
    vkUnmapMemory(vDevice, StageBufferPack.Memory);

    Common::createBuffer(vPhysicalDevice, vDevice, vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, voBuffer, voBufferMemory);

    VkCommandBuffer CommandBuffer = beginSingleTimeBuffer();
    Common::copyBuffer(CommandBuffer, StageBufferPack.Buffer, voBuffer, vSize);
    endSingleTimeBuffer(CommandBuffer);

    StageBufferPack.destroy(vDevice);
}

void Common::stageFillImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, const void* vData, VkDeviceSize vSize, VkImageCreateInfo vImageInfo, VkImage& voImage, VkDeviceMemory& voBufferMemory)
{
    uint32_t Width = vImageInfo.extent.width;
    uint32_t Height = vImageInfo.extent.height;
    uint32_t LayerCount = vImageInfo.arrayLayers;

    Common::SBufferPack StageBufferPack;
    Common::createBuffer(vPhysicalDevice, vDevice, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StageBufferPack.Buffer, StageBufferPack.Memory);

    void* pDevData;
    ck(vkMapMemory(vDevice, StageBufferPack.Memory, 0, vSize, 0, &pDevData));
    memcpy(pDevData, vData, vSize);
    vkUnmapMemory(vDevice, StageBufferPack.Memory);

    Common::createImage(vPhysicalDevice, vDevice, vImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, voImage, voBufferMemory);

    VkCommandBuffer CommandBuffer = beginSingleTimeBuffer();
    Common::transitionImageLayout(CommandBuffer, voImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, LayerCount);
    Common::copyBufferToImage(CommandBuffer, StageBufferPack.Buffer, voImage, Width, Height, LayerCount);
    Common::transitionImageLayout(CommandBuffer, voImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, LayerCount);
    endSingleTimeBuffer(CommandBuffer);

    StageBufferPack.destroy(vDevice);
}

Common::beginSingleTimeBufferFunc_t g_BeginFunc = nullptr;
Common::endSingleTimeBufferFunc_t g_EndFunc = nullptr;

void Common::setSingleTimeBufferFunc(Common::beginSingleTimeBufferFunc_t vBeginFunc, Common::endSingleTimeBufferFunc_t vEndFunc)
{
    g_BeginFunc = vBeginFunc;
    g_EndFunc = vEndFunc;
}

VkCommandBuffer Common::beginSingleTimeBuffer()
{
    _ASSERTE(g_BeginFunc != nullptr);
    return g_BeginFunc();
}

void Common::endSingleTimeBuffer(VkCommandBuffer vCommandBuffer)
{
    _ASSERTE(g_EndFunc != nullptr);
    g_EndFunc(vCommandBuffer);
}