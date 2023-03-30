#include "Function.h"
#include "Vulkan.h"
#include "IOImage.h"

using namespace vk;

VkFormat toVulkanFormat(EPixelFormat vPixelFormat)
{
    switch (vPixelFormat)
    {
    case EPixelFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
    case EPixelFormat::RGBA32: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case EPixelFormat::UNKNOWN:
    default:
        throw std::runtime_error(u8"暂未支持的图片格式");
        return VK_FORMAT_UNDEFINED;
    }
}

void Function::createImageFromIOImage(vk::CImage& voImage, CDevice::CPtr vDevice, CIOImage::CPtr vImage, int vMipLevel)
{
    _ASSERTE(vImage->getData());
    VkDeviceSize DataSize = vImage->getDataSize();

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(vImage->getWidth());
    ImageInfo.extent.height = static_cast<uint32_t>(vImage->getHeight());
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = vMipLevel;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = toVulkanFormat(vImage->getPixelFormat());
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (vMipLevel > 1)
        ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    voImage.create(vDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
    VkCommandBuffer CommandBuffer = vk::beginSingleTimeBuffer();
    voImage.stageFill(vImage->getData(), DataSize, vMipLevel > 1 ? false : true);
    if (vMipLevel > 1)
        voImage.generateMipmaps(CommandBuffer);
    vk::endSingleTimeBuffer(CommandBuffer);
}

void Function::createPlaceholderImage(vk::CImage& voImage, CDevice::CPtr vDevice)
{
    // placeholder image
    uint8_t Data[4] = { 0, 0, 0, 0 };
    CIOImage::Ptr pTinyImage = make<CIOImage>();
    pTinyImage->setSize(1, 1);
    pTinyImage->setData(Data);
    Function::createImageFromIOImage(voImage, vDevice, pTinyImage);
}

void Function::createDepthImage(vk::CImage& voImage, CDevice::CPtr vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage, VkFormat vFormat)
{
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = vExtent.width;
    ImageInfo.extent.height = vExtent.height;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = vFormat;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | vUsage;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

    voImage.create(vDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);

    VkCommandBuffer CommandBuffer = beginSingleTimeBuffer();
    voImage.transitionLayout(CommandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    endSingleTimeBuffer(CommandBuffer);
}

void Function::createImage2d(CImage& voImage, CDevice::CPtr vDevice, VkExtent2D vExtent, VkFormat vFormat, VkImageUsageFlags vUsage)
{
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(vExtent.width);
    ImageInfo.extent.height = static_cast<uint32_t>(vExtent.height);
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = vFormat;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = vUsage;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    voImage.create(vDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
}
