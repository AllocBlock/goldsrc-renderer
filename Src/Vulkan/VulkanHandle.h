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
                //throw "Vulkan对象在销毁前被析构";
                _ASSERT(false);
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

    // TIPS: CPointerSet manage a set of pointer, and you can not create instance by yourself,
    // all instance are created and destroyed inside this class
    template <typename T>
    struct CPointerSet
    {
    public:
        const ptr<T>& operator [](size_t vIndex) const
        {
            return m_Set[vIndex];
        }

        // auto destroy before init
        void init(size_t vNum)
        {
            destroyAndClearAll();
            m_Set.clear();
            m_Set.resize(vNum);
            for (auto& pV : m_Set)
                pV = make<T>();
        }

        size_t size() const { return m_Set.size(); }
        bool empty() const { return m_Set.empty(); }

        bool isValid(size_t vIndex) const
        {
            if (vIndex >= m_Set.size()) return false;
            return m_Set[vIndex]->isValid();
        }

        bool isAllValid() const
        {
            for (auto pHandle : m_Set)
            {
                if (!pHandle->isValid()) return false;
            }
            return true;
        }

        void destroyAndClearAll()
        {
            for (auto pHandle : m_Set)
            {
                pHandle->destroy();
            }
            m_Set.clear();
        }

    private:
        std::vector<ptr<T>> m_Set;
    };
}

#endif