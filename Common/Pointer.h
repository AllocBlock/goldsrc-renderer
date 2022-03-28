#include <memory>

namespace Pointer
{
    template <class T>
    using ptr = std::shared_ptr<T>;
#ifndef _MAKE_PTR
#define _MAKE_PTR
    template <class T, class... _Types>
    ptr<T> make(_Types&&... _Args) { return std::make_shared<T>(_Args...); }
#endif

#ifndef _DEFINE_NEW_FUNC
#define _DEFINE_NEW_FUNC(Class) static Ptr New() { return make<Class>(); }
#endif

#ifndef _DEFINE_PTR
#define _DEFINE_PTR(Class) using Ptr = ptr<Class>
#endif
}

using namespace Pointer;
