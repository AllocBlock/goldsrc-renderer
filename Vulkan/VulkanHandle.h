#pragma once
#include <vulkan/vulkan.h>
#include <iostream>

namespace vk
{
    template <typename T>
    class IVulkanHandle
    {
    public:
        virtual ~IVulkanHandle()
        {
            if (m_Handle)
                throw "Vulkan对象在销毁前被析构";
        }

        T get() { return m_Handle; }
    protected:
        T m_Handle;
    };
}


