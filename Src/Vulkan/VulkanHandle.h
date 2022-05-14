#pragma once
#ifndef _VULKAN_HANDLE_H
#define _VULKAN_HANDLE_H

#include <iostream>
#include "Pointer.h"

namespace vk
{
    template <typename T>
    class IVulkanHandle
    {
    public:
        virtual ~IVulkanHandle()
        {
            if (get())
                throw "Vulkan对象在销毁前被析构";
        }

        T get() const { return m_Handle; }
        const T* getConstPtr() const { return &m_Handle; }
        bool isValid() const { return get() != VK_NULL_HANDLE; }

        operator T() const { return get(); }
    protected:
        T* _getPtr() { return &m_Handle; }
        void _set(T vHandle) { m_Handle = vHandle; }
        void _setNull() { m_Handle = VK_NULL_HANDLE; }

    private:
        T m_Handle;
    };
}

#endif