#pragma once
#include "Pointer.h"
#include "IOImage.h"
#include <map>

enum class EIcon
{
    TIP = 0,
    MAX_NUM
};

class CIconManager
{
public:
    static ptr<CIconManager> getInstance()
    {
        if (!pInstance)
        {
            pInstance.reset(new CIconManager);
        }
        return pInstance;
    }

    CIOImage::CPtr getImage(EIcon vIcon) { return m_IconImageMap.at(vIcon); }

private:
    static ptr<CIconManager> pInstance;
    CIconManager();

    std::map<EIcon, CIOImage::Ptr> m_IconImageMap;
};