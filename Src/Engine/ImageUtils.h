#pragma once
#include "Image.h"

class CIOImage; // FIXME: manage reference to avoid this kind of pre-declaration

namespace ImageUtils
{
    void createImageFromIOImage(vk::CImage& voImage, vk::CDevice::CPtr vDevice, cptr<CIOImage> vImage, int vMipLevel = 1);
    void createPlaceholderImage(vk::CImage& voImage, vk::CDevice::CPtr vDevice);
    void createDepthImage(vk::CImage& voImage, vk::CDevice::CPtr vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage = NULL, VkFormat vFormat = VkFormat::VK_FORMAT_D32_SFLOAT);
    void createImage2d(vk::CImage& voImage, vk::CDevice::CPtr vDevice, VkExtent2D vExtent, VkFormat vFormat, VkImageUsageFlags vUsage);
}