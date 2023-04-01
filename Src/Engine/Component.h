#pragma once
#include "Common.h"
#include "Transform.h"

class IComponent
{
public:
    _DEFINE_GETTER_SETTER_POINTER(Parent, ptr<STransform>);
    ptr<STransform> getTransform() { return m_pParent; }
    void attachTo(ptr<STransform> vTransform) { m_pParent = vTransform; }

private:
    ptr<STransform> m_pParent = nullptr;
};