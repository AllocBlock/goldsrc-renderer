#pragma once
#include "PchVulkan.h"
#include "Buffer.h"

namespace vk
{
    class CUniformBuffer : public vk::CBuffer
    {
    public:
        _DEFINE_PTR(CUniformBuffer);

        void create(CDevice::CPtr vDevice, VkDeviceSize vSize);
        void update(const void* vData);
    };
}

