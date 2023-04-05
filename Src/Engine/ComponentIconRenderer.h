#pragma once
#include "Component.h"
#include "IconManager.h"
#include <optional>

class CComponentIconRenderer : public IComponent
{
public:
    bool hasIcon() const { return m_Icon.has_value(); }
    EIcon getIcon() const { return m_Icon.value(); }
    void setIcon(EIcon vIcon) { m_Icon = vIcon; }
    
    virtual SAABB getAABBV() const override
    {
        return SAABB::createByCenterExtent(m_pParent.lock()->getAbsoluteTranslate(), m_pParent.lock()->getAbsoluteScale() * 2.0f);
    }

protected:
    virtual std::string _getNameV() const override { return "Icon Renderer"; }

private:
    std::optional<EIcon> m_Icon = std::nullopt;
};
