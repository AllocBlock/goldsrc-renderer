#pragma once
#include "IOImage.h"
#include "Image.h"
#include <vulkan/vulkan.h>

namespace Function
{
    void createImageFromIOImage(vk::CImage& voImage, vk::CDevice::CPtr vDevice, CIOImage::CPtr vImage, int vMipLevel = 1);
    void createPlaceholderImage(vk::CImage& voImage, vk::CDevice::CPtr vDevice);
    void createDepthImage(vk::CImage& voImage, vk::CDevice::CPtr vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage = NULL, VkFormat vFormat = VkFormat::VK_FORMAT_D32_SFLOAT);
}