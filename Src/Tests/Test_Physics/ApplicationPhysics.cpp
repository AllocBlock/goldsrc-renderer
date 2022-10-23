#include "ApplicationPhysics.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"
#include "Ticker.h"

#include <random>

using namespace vk;

CTempScene<CMeshDataGeneral>::Ptr __generateScene()
{
    auto m_pScene = make<CTempScene<CMeshDataGeneral>>();

    // Ground
    {
        auto pGroundMesh = make<CMeshBasicQuad>();
        auto pGroundActor = make<CActor<CMeshDataGeneral>>("Ground");
        pGroundActor->setMesh(pGroundMesh);
        pGroundActor->getPhysicsState()->IsStatic = true;
        pGroundActor->getPhysicsState()->pCollider = make<CColliderBasic>(pGroundActor->getTransform(), EBasicColliderType::PLANE);

        m_pScene->addActor(pGroundActor);
    }

    //// Cubes
    //{
    //    auto pCubeMesh1 = make<CMeshBasicCube>();
    //    auto pCubeActor1 = make<CActor>("Cube1");
    //    pCubeActor1->setMesh(pCubeMesh1);
    //    pCubeActor1->getPhysicsState()->pCollider = make<CColliderBasic>(pCubeActor1->getTransform(), EBasicColliderType::CUBE);

    //    m_pScene->addActor(pCubeActor1);
    //}

    // Quad
    {
        auto pQuadMesh1 = make<CMeshBasicQuad>();
        auto pQuadActor1 = make<CActor<CMeshDataGeneral>>("Quad1");
        pQuadActor1->setMesh(pQuadMesh1);
        pQuadActor1->getPhysicsState()->pCollider = make<CColliderBasic>(pQuadActor1->getTransform(), EBasicColliderType::QUAD);

        m_pScene->addActor(pQuadActor1);
    }

    // Spheres
    /*{
        auto pSphereMesh1 = make<CMeshBasicSphere>();
        auto pSphereActor1 = make<CActor>("Sphere1");
        pSphereActor1->setMesh(pSphereMesh1);
        pSphereActor1->getPhysicsState()->pCollider = make<CColliderBasic>(pSphereActor1->getTransform(), EBasicColliderType::SPHERE);

        m_pScene->addActor(pSphereActor1);
    }*/

    return m_pScene;
}

glm::vec3 __generateRandomUpForce(float vIntensity = 20000.0f)
{
    static std::default_random_engine e;
    static std::uniform_real_distribution<float> u(-1, 1);
    return glm::normalize(glm::vec3(u(e), u(e), abs(u(e)))) * vIntensity;
}

glm::vec3 __generateRandomAlpha()
{
    static std::default_random_engine e;
    static std::uniform_real_distribution<float> u(-1, 1);
    float Speed = 2000.0f * (u(e) * 0.5f + 0.5f);
    return glm::normalize(glm::vec3(u(e), u(e), u(e))) * Speed;
}

void CApplicationPhysics::_createV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    auto AppInfo = getAppInfo();

    m_pCamera = make<CCamera>();
    m_pCamera->setFov(90);
    m_pCamera->setAspect(AppInfo.Extent.width, AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(20.0, 20.0, 20.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    m_pPhysicsEngine = make<CPhysicsEngine>();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    m_pPassShade = make<CRenderPassShade>();
    m_pPassShade->init(AppInfo);
    m_pPassShade->setCamera(m_pCamera);

    m_pPassVisPhysics = make<CRenderPassVisPhysics>();
    m_pPassVisPhysics->init(AppInfo);
    m_pPassVisPhysics->setCamera(m_pCamera);

    m_pScene = __generateScene();
    __resetActors();
    __initPhysicsEngine();
    m_pPassVisPhysics->setPhysicsEngine(m_pPhysicsEngine);
    m_pPassShade->setScene(m_pScene);

    __linkPasses();
}

void CApplicationPhysics::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassShade->update(vImageIndex);
    m_pPassVisPhysics->update(vImageIndex);

    // FIXME: move static to member? or global?
    static CTicker Ticker;
    float DeltaTime = Ticker.update();
    m_pPhysicsEngine->update(DeltaTime);
}

void CApplicationPhysics::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"物理系统 Physics");
    UI::text(u8"测试");
    m_pInteractor->getCamera()->renderUI();

    m_pPassShade->renderUI();
    m_pPassVisPhysics->renderUI();

    // physics engine
    {
        if (UI::collapse(u8"物理引擎", true))
        {
            bool Paused = m_pPhysicsEngine->isPaused();
            UI::toggle(u8"暂停模拟", Paused);
            if (Paused != m_pPhysicsEngine->isPaused())
                if (Paused) m_pPhysicsEngine->pause();
                else m_pPhysicsEngine->resume();

            float Speed = m_pPhysicsEngine->getSimulateSpeed();
            UI::drag(u8"模拟速度", Speed, 0.01f, 0.01f, 2.0f);
            m_pPhysicsEngine->setSimulateSpeed(Speed);

            bool EnableGravity = m_pPhysicsEngine->isGravityEnabled();
            UI::toggle(u8"开启重力", EnableGravity);
            m_pPhysicsEngine->setGravityState(EnableGravity);
        }
    }

    // scene
    {
        if (UI::collapse(u8"场景", true))
        {
            UI::indent(20.0f);
            if (UI::button(u8"重置场景"))
                __resetActors();
            if (UI::button(u8"随机力"))
            {
                for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
                    m_pScene->getActor(i)->getPhysicsState()->addForce(__generateRandomUpForce());
            }

            if (UI::button(u8"随机角加速度"))
            {
                for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
                    m_pScene->getActor(i)->getPhysicsState()->addAlpha(__generateRandomAlpha());
            }

            for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
            {
                auto pActor = m_pScene->getActor(i);
                std::string ActorName = pActor->getName();
                if (UI::collapse(ActorName + "##Scene"))
                {
                    UI::indent(20.0f);
                    auto pTransform = pActor->getTransform();
                    UI::drag(u8"位置##" + ActorName + u8"_Scene", pTransform->Translate);
                    glm::vec3 Euler = pTransform->Rotate.getEulerDegrees();
                    UI::drag(u8"旋转##" + ActorName + u8"_Scene", Euler, 0.1f);
                    pTransform->Rotate.setEulerDegrees(Euler);
                    UI::drag(u8"缩放##" + ActorName + u8"_Scene", pTransform->Scale);
                    
                    UI::toggle(u8"为静态物体##" + ActorName + u8"_Scene", pActor->getPhysicsState()->IsStatic);
                    UI::toggle(u8"开启重力##" + ActorName + u8"_Scene", pActor->getPhysicsState()->HasGravity);
                    UI::drag(u8"质量##" + ActorName + u8"_Scene", pActor->getPhysicsState()->Mass, 0.01f, 0.01f);
                    UI::drag(u8"速度##" + ActorName + u8"_Scene", pActor->getPhysicsState()->Velocity);
                    UI::drag(u8"角速度##" + ActorName + u8"_Scene", pActor->getPhysicsState()->AngularVelocity);
                    UI::unindent();
                }
            }
            UI::unindent();
        }
    }
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationPhysics::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> ShadeBuffers = m_pPassShade->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> VisPhysicsBuffers = m_pPassVisPhysics->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = ShadeBuffers;
    Result.insert(Result.end(), VisPhysicsBuffers.begin(), VisPhysicsBuffers.end());
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationPhysics::_destroyV()
{
    destroyAndClear(m_pPassGUI);
    destroyAndClear(m_pPassShade);
    destroyAndClear(m_pPassVisPhysics);
    m_pPhysicsEngine = nullptr;

    cleanGlobalCommandBuffer();
}

void CApplicationPhysics::__initPhysicsEngine()
{
    for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
    {
        auto pActor = m_pScene->getActor(i);
        m_pPhysicsEngine->addRigidBody(pActor->getPhysicsState());
    }
}

void CApplicationPhysics::__resetActors()
{
    auto pGround = m_pScene->findActor("Ground");
    _ASSERTE(pGround);
    pGround->setScale(20.0f);

    /*auto pCube1 = m_pScene->findActor("Cube1");
    _ASSERTE(pCube1);
    pCube1->resetTransform();
    pCube1->setTranslate(glm::vec3(0.0f, 0.0f, 16.0f));
    pCube1->setScale(3.0f);
    pCube1->clearMoveState();*/

    auto pQuad1 = m_pScene->findActor("Quad1");
    _ASSERTE(pQuad1);
    pQuad1->resetTransform();
    pQuad1->setTranslate(glm::vec3(0.0f, 0.0f, 16.0f));
    pQuad1->setScale(2.0f);
    pQuad1->setRotate(CRotator(glm::vec3(50, 30, 0)));
    pQuad1->clearMoveState();

 /*   auto pSphere1 = m_pScene->findActor("Sphere1");
    _ASSERTE(pSphere1);
    pSphere1->resetTransform();
    pSphere1->setTranslate(glm::vec3(0.0f, 6.0f, 20.0f));
    pSphere1->clearMoveState();*/
}

void CApplicationPhysics::__linkPasses()
{
    auto pPortShade = m_pPassShade->getPortSet();
    auto pPortVisPhysics = m_pPassVisPhysics->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortShade, "Main");;
    CPortSet::link(pPortShade,  "Main", pPortVisPhysics, "Main");
    CPortSet::link(pPortVisPhysics,  "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
}