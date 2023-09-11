#pragma once
#include "Common.h"

class CGuiWindow
{
public:
    ~CGuiWindow() = default;

    _DEFINE_GETTER_SETTER(Show, bool);
    void renderUI();

protected:
    virtual std::string _getWindowNameV() = 0;
    virtual void _renderUIV() = 0;
private:
    bool m_Show = true;
};