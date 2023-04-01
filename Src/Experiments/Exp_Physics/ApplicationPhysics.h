#pragma once
#include "Application.h"
#include "Interactor.h"
#include "Scene.h"
#include "PhysicsEngine.h"
#include "PassShade.h"
#include "PassVisPhysics.h"
#include "PassGUI.h"

class CApplicationPhysics : public IApplication
{
public:
    CApplicationPhysics() = default;

protected:
    virtual void _createV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

private:
    void __initPhysicsEngine();
    void __resetActors();
    void __linkPasses();

    CCamera::Ptr m_pCamera = nullptr;

    ptr<CRenderPassGUI> m_pPassGUI = nullptr;
    ptr<CRenderPassShade> m_pPassShade = nullptr;
    ptr<CRenderPassVisPhysics> m_pPassVisPhysics = nullptr;

    ptr<CInteractor> m_pInteractor = nullptr;

    CScene<CMeshData>::Ptr m_pScene = nullptr;
    CPhysicsEngine::Ptr m_pPhysicsEngine = nullptr;
};
