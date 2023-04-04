#pragma once
#include "Common.h"

class CTransform; // avoid mutual include
class IComponent
{
public:
    _DEFINE_PTR(IComponent);
    virtual ~IComponent() = default;

    std::string getName() const { return _getNameV(); }
    ptr<CTransform> getTransform() const { return m_pParent.expired() ? nullptr : m_pParent.lock(); }

protected:
    virtual std::string _getNameV() const = 0;

protected:
    void __setParent(wptr<CTransform> vTransform)
    {
        _ASSERTE(m_pParent.expired()); // do not allow change of parent for now
        m_pParent = vTransform;
    }

    friend CTransform;
    wptr<CTransform> m_pParent;
};