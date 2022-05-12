#pragma once
#include "IOImage.h"
#include "Image.h"
#include <vulkan/vulkan.h>

namespace Function
{
    vk::CImage::Ptr createImageFromIOImage(vk::CPhysicalDevice::CPtr vPhysicalDevice, vk::CDevice::CPtr vDevice, CIOImage::CPtr vImage, int vMipLevel = 1);
    vk::CImage::Ptr createPlaceholderImage(vk::CPhysicalDevice::CPtr vPhysicalDevice, vk::CDevice::CPtr vDevice);
    vk::CImage::Ptr createDepthImage(vk::CPhysicalDevice::CPtr vPhysicalDevice, vk::CDevice::CPtr vDevice, VkExtent2D vExtent, VkImageUsageFlags vUsage = NULL, VkFormat vFormat = VkFormat::VK_FORMAT_D32_SFLOAT);
}