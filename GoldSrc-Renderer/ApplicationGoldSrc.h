#pragma once
#include "ApplicationBase.h"
#include "ImguiMain.h"
#include "SceneInteractor.h"

class CApplicationGoldSrc : public CApplicationBase
{
public:
    CApplicationGoldSrc() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    std::shared_ptr<CGUIMain> m_pGUI = nullptr;
    std::shared_ptr<CSceneInteractor> m_pInteractor = nullptr;
};

