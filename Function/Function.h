#pragma once
#include "IOImage.h"
#include "Image.h"
#include <vulkan/vulkan.h>

namespace Function
{
    vk::CImage::Ptr createImageFromIOImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, ptr<CIOImage> vImage);
}