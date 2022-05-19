#pragma once
#include "IApplication.h"
#include "ScenePass.h"
#include "GUIPass.h"
#include "PassLine.h"
#include "ImguiMain.h"
#include "Interactor.h"
#include "Scene.h"

class CApplicationGoldSrc : public IApplication
{
public:
    CApplicationGoldSrc() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    void __recreateRenderer(ERenderMethod vMethod = ERenderMethod::BSP);
    void __linkPasses();

    ptr<CSceneRenderPass> m_pPassScene = nullptr;
    ptr<CGUIRenderPass> m_pPassGUI = nullptr;
    ptr<CLineRenderPass> m_pPassLine = nullptr;
    ptr<CGUIMain> m_pMainUI = nullptr;
    ptr<CInteractor> m_pInteractor = nullptr;
    ptr<CCamera> m_pCamera = nullptr;

    ptr<SScene> m_pScene = nullptr;
};

