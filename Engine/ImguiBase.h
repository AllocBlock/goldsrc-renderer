#pragma once
class CImguiBase
{
public:
    CImguiBase() = default;
    void renderUI() { _renderUIV(); }

protected:
    virtual void _renderUIV() = 0;
};

