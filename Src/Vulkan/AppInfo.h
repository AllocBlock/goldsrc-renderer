#pragma once

#include "Image.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Surface.h"
#include "Instance.h"

namespace vk
{
    struct SAppInfo
    {
        SAppInfo() { clear(); }
        CDevice::CPtr pDevice;
        VkExtent2D Extent;
        VkFormat ImageFormat;
        size_t ImageNum;

        void clear()
        {
            pDevice = nullptr;
            Extent = { 0, 0 };
            ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
            ImageNum = 0;
        }
    };
}