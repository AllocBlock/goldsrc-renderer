#pragma once
class IGUI
{
public:
    IGUI() = default;
    virtual ~IGUI() = default;

    void renderUI() { _renderUIV(); }

protected:
    virtual void _renderUIV() = 0;
};

