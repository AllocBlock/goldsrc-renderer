#pragma once
#include <memory>
#ifndef _POINTER_H
#define _POINTER_H

namespace Pointer
{
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using cptr = std::shared_ptr<const T>;

    template <class T>
    using wptr = std::weak_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

    template <class T, class... _Types>
    sptr<T> make(_Types&&... _Args) { return std::make_shared<T>(_Args...); }

    template <typename T>
    bool isNonEmptyAndValid(const sptr<const T>& vPointer)
    {
        return vPointer && vPointer->isValid();
    }

    template <typename T>
    bool isNonEmptyAndValid(const sptr<T>& vPointer)
    {
        return vPointer && vPointer->isValid();
    }

    template <typename T>
    void destroyAndClear(sptr<T>& vPointer) 
    { 
        if (vPointer)
        {
            vPointer->destroy();
            vPointer = nullptr;
        }
    }
}

using namespace Pointer;

#endif