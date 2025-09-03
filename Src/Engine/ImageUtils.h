#pragma once
#include "Image.h"

class CIOImage; // FIXME: manage reference to avoid this kind of pre-declaration

namespace ImageUtils
{
    void createImageFromIOImage(vk::CImage& voImage, cptr<vk::CDevice> vDevice, cptr<CIOImage> vImage, int vMipLevel = 1);
    void createPlaceholderImage(vk::CImage& voImage, cptr<vk::CDevice> vDevice);
    void createImage2d(vk::CImage& voImage, cptr<vk::CDevice> vDevice, VkExtent2D vExtent, VkFormat vFormat, VkImageUsageFlags vUsage);
    void createDepthImage(vk::CImage& voImage, cptr<vk::CDevice> vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage = NULL, VkFormat vFormat = VkFormat::VK_FORMAT_D24_UNORM_S8_UINT);
}