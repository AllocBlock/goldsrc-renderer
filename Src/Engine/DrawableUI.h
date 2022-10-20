#pragma once
class IDrawableUI
{
public:
    IDrawableUI() = default;
    virtual ~IDrawableUI() = default;

    void renderUI() { _renderUIV(); }

protected:
    virtual void _renderUIV() = 0;
};

