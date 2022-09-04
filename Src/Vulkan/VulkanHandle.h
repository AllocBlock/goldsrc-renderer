#pragma once
#ifndef _VULKAN_HANDLE_H
#define _VULKAN_HANDLE_H

#include "Pointer.h"
#include <iostream>
#include <vector>

namespace vk
{
    template <typename T>
    class IVulkanHandle
    {
    public:
        virtual ~IVulkanHandle()
        {
#ifdef _DEBUG
            if (get())
            {
                throw "Vulkan对象在销毁前被析构";
            }
#endif
        }
        IVulkanHandle() = default;

        // forbid copy
        //IVulkanHandle(IVulkanHandle<T>&) = delete;
        //IVulkanHandle<T>& operator=(IVulkanHandle<T>&) = delete;

        T get() const { return m_Handle; }
        const T* getConstPtr() const { return &m_Handle; }
        virtual bool isValid() const { return get() != VK_NULL_HANDLE; }

        operator T() const { return get(); }
    protected:
        T* _getPtr() { return &m_Handle; }
        T& _getRef() { return m_Handle; }
        void _set(T vHandle) { m_Handle = vHandle; }
        void _setNull() { m_Handle = VK_NULL_HANDLE; }

    private:
        T m_Handle = VK_NULL_HANDLE;
    };

    template <typename T>
    struct CHandleSet
    {
    public:
        T& operator [](size_t vIndex)
        {
            return m_Set[vIndex];
        }

        const T& operator [](size_t vIndex) const
        {
            return m_Set[vIndex];
        }

        // auto destroy before init
        void init(size_t vNum)
        {
            destroyAndClearAll();
            m_Set.clear();
            m_Set.resize(vNum);
        }

        size_t size() const { return m_Set.size(); }
        bool empty() const { return m_Set.empty(); }

        bool isValid(size_t vIndex) const
        {
            if (vIndex >= m_Set.size()) return false;
            return m_Set[vIndex].isValid();
        }

        bool isAllValid() const
        {
            for (const T& Handle : m_Set)
            {
                if (!Handle.isValid()) return false;
            }
            return true;
        }

        void destroyAndClearAll()
        {
            for (T& Handle : m_Set)
            {
                Handle.destroy();
            }
            m_Set.clear();
        }

    private:
        std::vector<T> m_Set;
    };
}

#endif