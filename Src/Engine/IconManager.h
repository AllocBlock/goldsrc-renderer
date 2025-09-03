#pragma once
#include "Pointer.h"
#include "IOImage.h"
#include <map>

enum class EIcon
{
    TIP = 0,
    QUESTION,
    ALERT,
    MAX_NUM
};

enum class EIconRenderType
{
    OPAQUE = 0,
    INDEXED_TRANSPARENT,
    ALPHA,
};

class CIconManager
{
public:
    static sptr<CIconManager> getInstance()
    {
        if (!pInstance)
        {
            pInstance.reset(new CIconManager);
        }
        return pInstance;
    }

    cptr<CIOImage> getImage(EIcon vIcon) const { return m_IconImageMap.at(vIcon).pImage; }
    EIconRenderType getRenderType(EIcon vIcon) const { return m_IconImageMap.at(vIcon).RenderType; }

private:
    static sptr<CIconManager> pInstance;
    CIconManager();

    struct IconInfo
    {
        sptr<CIOImage> pImage;
        EIconRenderType RenderType;
    };

    std::map<EIcon, IconInfo> m_IconImageMap;
};