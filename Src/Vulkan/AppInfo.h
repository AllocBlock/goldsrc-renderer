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
        CInstance::CPtr pInstance;
        CPhysicalDevice::CPtr pPhysicalDevice;
        CDevice::CPtr pDevice;
        uint32_t GraphicsQueueIndex;
        VkQueue GraphicsQueue;
        VkExtent2D Extent;
        VkFormat ImageFormat;
        size_t ImageNum;

        void clear()
        {
            pInstance = nullptr;
            pPhysicalDevice = nullptr;
            pDevice = nullptr;
            GraphicsQueueIndex = std::numeric_limits<uint32_t>::max();
            GraphicsQueue = VK_NULL_HANDLE;
            Extent = { 0, 0 };
            ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
            ImageNum = 0;
        }
    };
}