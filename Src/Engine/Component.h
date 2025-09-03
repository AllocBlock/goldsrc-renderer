#pragma once
#include "Pointer.h"
#include "Common.h"
#include "BoundingBox.h"
#include "DrawableUI.h"

class CTransform; // avoid mutual include
class IComponent : public IDrawableUI
{
public:
        virtual ~IComponent() = default;

    std::string getName() const { return _getNameV(); }
    sptr<CTransform> getTransform() const { return m_pParent.expired() ? nullptr : m_pParent.lock(); }

    virtual void _renderUIV() {}
    virtual SAABB getAABBV() const = 0;

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