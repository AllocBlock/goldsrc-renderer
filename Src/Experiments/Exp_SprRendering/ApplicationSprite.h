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

    sptr<CCamera> m_pCamera = nullptr;

    sptr<CRenderPassGUI> m_pPassGUI = nullptr;
    sptr<CRenderPassSprite> m_pPassSprite = nullptr;
    sptr<CInteractor> m_pInteractor = nullptr;
};
