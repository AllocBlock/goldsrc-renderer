#pragma once

#include <filesystem>

#define _SHOULD_NOT_GO_HERE throw std::runtime_error("Should not go here")

#define _DEFINE_GETTER(Name, Type) const Type& get##Name() const { return m_##Name; }
#define _DEFINE_SETTER(Name, Type) void set##Name(const Type& v) { m_##Name = v; }
#define _DEFINE_GETTER_SETTER(Name, Type) _DEFINE_GETTER(Name, Type) _DEFINE_SETTER(Name, Type)

#define _DEFINE_GETTER_POINTER(Name, Type) const Type& get##Name() const { return m_p##Name; }
#define _DEFINE_SETTER_POINTER(Name, Type) void set##Name(const Type& v) { m_p##Name = v; }
#define _DEFINE_GETTER_SETTER_POINTER(Name, Type) _DEFINE_GETTER_POINTER(Name, Type) _DEFINE_SETTER_POINTER(Name, Type)

namespace Common
{
    float mod(float vVal, float vMax);
    std::vector<char> readFileAsChar(std::filesystem::path vFilePath);

    template <typename T>
    T lerp(T vA, T vB, float vFactor)
    {
        return static_cast<T>(vA * vFactor + vB * (1 - vFactor));
    }
}
