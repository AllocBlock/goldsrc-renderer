#pragma once

#include "Device.h"
#include "Common.h"
#include "Event.h"

class CAppInfo
{
    _DEFINE_UPDATE_EVENT(ImageNum, uint32_t)
    _DEFINE_UPDATE_EVENT(ScreenExtent, VkExtent2D)
public:
    _DEFINE_PTR(CAppInfo);
    
    _DEFINE_GETTER(ImageNum, uint32_t)
    _DEFINE_GETTER(ScreenExtent, VkExtent2D)

    CAppInfo() { clear(); }

private:

    void setImageNum(uint32_t vIndexNum) { if (vIndexNum != m_ImageNum) { m_ImageNum = vIndexNum; m_ImageNumUpdateEventHandler.trigger(m_ImageNum); } }
    void setScreenExtent(VkExtent2D vExtent) { if (vExtent != m_ScreenExtent) { m_ScreenExtent = vExtent; m_ScreenExtentUpdateEventHandler.trigger(vExtent); } }

    void clear()
    {
        m_ImageNum = 0;
        m_ScreenExtent = { 0, 0 };
    }

    uint32_t m_ImageNum;
    VkExtent2D m_ScreenExtent;

    friend class IApplication; // only application can set, others can only get and hook
};
