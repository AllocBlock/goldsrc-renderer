#pragma once
#include "Application.h"
#include "Interactor.h"
#include "PassGUI.h"
#include "PassSprite.h"

class CApplicationSprite : public IApplication
{
public:
    CApplicationSprite() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual void _destroyV() override;

private:
    void __linkPasses();

    CCamera::Ptr m_pCamera = nullptr;

    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassSprite> m_pPassSprite = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
};
