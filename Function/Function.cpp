#include "Function.h"
#include "Vulkan.h"

VkFormat toVulkanFormat(EPixelFormat vPixelFormat)
{
    switch (vPixelFormat)
    {
    case EPixelFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
    case EPixelFormat::RGBA32: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case EPixelFormat::UNKNOWN:
    default:
        break;
    }
}

vk::CImage::Ptr Function::createImageFromIOImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, ptr<CIOImage> vImage)
{
    _ASSERTE(vImage->getData());
    VkDeviceSize DataSize = vImage->getDataSize();

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(vImage->getWidth());
    ImageInfo.extent.height = static_cast<uint32_t>(vImage->getHeight());
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = toVulkanFormat(vImage->getPixelFormat());
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    ptr<vk::CImage> pImage2;
    vk::CImage::Ptr pImage = make<vk::CImage>();
    pImage->create(vPhysicalDevice, vDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    pImage->stageFill(vImage->getData(), DataSize);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    return pImage;
}