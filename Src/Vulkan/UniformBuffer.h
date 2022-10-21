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

    template <typename SUBO_t>
    class CUniformBufferTyped : public vk::CBuffer
    {
    public:
        _DEFINE_PTR(CUniformBuffer);
        
        void create(CDevice::CPtr vDevice)
        {
            destroy();
            CBuffer::create(vDevice, sizeof(SUBO_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
        
        void update(const SUBO_t& vData)
        {
            if (!isValid()) throw "Cant fill in NULL handle buffer";
            fill(&vData, getSize());
        }
    };
}

