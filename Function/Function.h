#pragma once
#include "IOImage.h"
#include "Image.h"
#include <vulkan/vulkan.h>

namespace Function
{
    std::shared_ptr<vk::CImage> createImageFromIOImage(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, std::shared_ptr<CIOImage> vImage);
}