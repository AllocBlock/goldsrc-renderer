#pragma once
#include "GUIBase.h"

class CGUITest : public CGUIBase
{
public:
    CGUITest() = default;

protected:
    virtual void _updateV(uint32_t vImageIndex) override;

private:
    void __drawGUI();
};

