#include <memory>
#ifndef _POINTER_H
#define _POINTER_H

namespace Pointer
{
    template <class T>
    using ptr = std::shared_ptr<T>;

    template <class T>
    using cptr = std::shared_ptr<const T>;

    template <class T>
    using wptr = std::weak_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

#ifndef _MAKE_PTR
#define _MAKE_PTR
    template <class T, class... _Types>
    ptr<T> make(_Types&&... _Args) { return std::make_shared<T>(_Args...); }
#endif

#ifndef _DEFINE_PTR
#define _DEFINE_PTR(Class) using Ptr = ptr<Class>; using CPtr = ptr<const Class>
#endif

    template <typename T>
    bool isNonEmptyAndValid(const ptr<const T>& vPointer)
    {
        return vPointer && vPointer->isValid();
    }

    template <typename T>
    bool isNonEmptyAndValid(const ptr<T>& vPointer)
    {
        return vPointer && vPointer->isValid();
    }

    template <typename T>
    void destroyAndClear(ptr<T>& vPointer) 
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