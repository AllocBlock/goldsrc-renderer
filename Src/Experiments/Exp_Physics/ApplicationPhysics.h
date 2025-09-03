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

    sptr<CCamera> m_pCamera = nullptr;

    sptr<CRenderPassGUI> m_pPassGUI = nullptr;
    sptr<CRenderPassShade> m_pPassShade = nullptr;
    sptr<CRenderPassVisPhysics> m_pPassVisPhysics = nullptr;

    sptr<CInteractor> m_pInteractor = nullptr;

    CScene<CMeshData>::Ptr m_pScene = nullptr;
    sptr<CPhysicsEngine> m_pPhysicsEngine = nullptr;
};
