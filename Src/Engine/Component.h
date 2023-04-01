#pragma once
#include "Common.h"

class CTransform; // avoid mutual include
class IComponent
{
public:
    _DEFINE_PTR(IComponent);

    ptr<CTransform> getTransform() const { return m_pParent.expired() ? nullptr : m_pParent.lock(); }

protected:
    virtual std::string _getNameV() const = 0;

private:
    void __setParent(wptr<CTransform> vTransform)
    {
        _ASSERTE(m_pParent.expired()); // do not allow change of parent for now
        m_pParent = vTransform;
    }

    friend CTransform;
    wptr<CTransform> m_pParent;
};