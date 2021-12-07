#include "Function.h"
#include "Vulkan.h"

std::shared_ptr<vk::CImage> Function::createImageFromIOImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, std::shared_ptr<CIOImage> vImage)
{
    size_t TexWidth = vImage->getWidth();
    size_t TexHeight = vImage->getHeight();
    const void* pPixelData = vImage->getData();

    VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(TexWidth);
    ImageInfo.extent.height = static_cast<uint32_t>(TexHeight);
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    std::shared_ptr<vk::CImage> pImage = std::make_shared<vk::CImage>();
    pImage->create(vPhysicalDevice, vDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
    VkCommandBuffer CommandBuffer = Vulkan::beginSingleTimeBuffer();
    pImage->stageFill(pPixelData, DataSize);
    Vulkan::endSingleTimeBuffer(CommandBuffer);

    return pImage;
}