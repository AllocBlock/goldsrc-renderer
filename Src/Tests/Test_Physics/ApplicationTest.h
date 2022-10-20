#pragma once
#include "Application.h"
#include "Interactor.h"
#include "TempScene.h"
#include "PhysicsEngine.h"
#include "PassShade.h"
#include "PassVisPhysics.h"
#include "PassGUI.h"

class CApplicationTest : public IApplication
{
public:
    CApplicationTest() = default;

protected:
    virtual void _initV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _createOtherResourceV() override;
    virtual void _recreateOtherResourceV() override;
    virtual void _destroyOtherResourceV() override;

private:
    void __initPhysicsEngine();
    void __resetActors();
    void __linkPasses();

    ptr<CCamera> m_pCamera = nullptr;

    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassShade> m_pPassShade = nullptr;
    ptr<CRenderPassVisPhysics> m_pPassVisPhysics = nullptr;

    ptr<CInteractor> m_pInteractor = nullptr;

    CTempScene::Ptr m_pScene = nullptr;
    CPhysicsEngine::Ptr m_pPhysicsEngine = nullptr;
};
