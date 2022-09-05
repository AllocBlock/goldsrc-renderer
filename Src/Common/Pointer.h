#include <memory>
#ifndef _POINTER_H
#define _POINTER_H

namespace Pointer
{
    template <class T>
    using ptr = std::shared_ptr<T>;

    template <class T>
    using wptr = std::weak_ptr<T>;

#ifndef _MAKE_PTR
#define _MAKE_PTR
    template <class T, class... _Types>
    ptr<T> make(_Types&&... _Args) { return std::make_shared<T>(_Args...); }
#endif

#ifndef _DEFINE_PTR
#define _DEFINE_PTR(Class) using Ptr = ptr<Class>; using CPtr = ptr<const Class>
#endif

    class IPointerOnly
    {
    protected:
        IPointerOnly() = default;

        template <class T, class... _Types>
        friend ptr<T> make(_Types&&... _Args);
    };

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