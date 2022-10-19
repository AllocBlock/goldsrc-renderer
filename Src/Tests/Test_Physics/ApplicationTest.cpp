#include "ApplicationTest.h"
#include "GlobalSingleTimeBuffer.h"
#include "Gui.h"
#include "Ticker.h"

#include <random>

using namespace vk;

CTempScene::Ptr __generateScene()
{
    CTempScene::Ptr m_pScene = make<CTempScene>();

    // Ground
    {
        auto pGroundMesh = make<CMeshBasicQuad>();
        auto pGroundActor = make<CActor>("Ground");
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
        auto pQuadActor1 = make<CActor>("Quad1");
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
    float Speed = 2000.0f * (u(e) * 0.5 + 0.5);
    return glm::normalize(glm::vec3(u(e), u(e), u(e))) * Speed;
}

void CApplicationTest::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    auto AppInfo = getAppInfo();

    m_pCamera = make<CCamera>();
    m_pCamera->setFov(90);
    m_pCamera->setAspect(AppInfo.Extent.width / AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(20.0, 20.0, 20.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    m_pPhysicsEngine = make<CPhysicsEngine>();
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassShade->update(vImageIndex);

    // FIXME: move static to member? or global?
    static CTicker Ticker;
    float DeltaTime = Ticker.update();
    m_pPhysicsEngine->update(DeltaTime);
}

void CApplicationTest::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"����ϵͳ Physics");
    UI::text(u8"����");
    m_pInteractor->getCamera()->renderUI();

    m_pPassShade->renderUI();

    // physics engine
    {
        if (UI::collapse(u8"��������", true))
        {
            bool Paused = m_pPhysicsEngine->isPaused();
            UI::toggle(u8"��ͣģ��", Paused);
            if (Paused != m_pPhysicsEngine->isPaused())
                if (Paused) m_pPhysicsEngine->pause();
                else m_pPhysicsEngine->resume();

            float Speed = m_pPhysicsEngine->getSimulateSpeed();
            UI::drag(u8"ģ���ٶ�", Speed, 0.01f, 0.01f, 2.0f);
            m_pPhysicsEngine->setSimulateSpeed(Speed);

            bool EnableGravity = m_pPhysicsEngine->isGravityEnabled();
            UI::toggle(u8"��������", EnableGravity);
            m_pPhysicsEngine->setGravityState(EnableGravity);
        }
    }

    // scene
    {
        if (UI::collapse(u8"����", true))
        {
            UI::indent(20.0f);
            if (UI::button(u8"���ó���"))
                __resetActors();
            if (UI::button(u8"�����"))
            {
                for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
                    m_pScene->getActor(i)->getPhysicsState()->addForce(__generateRandomUpForce());
            }

            if (UI::button(u8"����Ǽ��ٶ�"))
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
                    UI::drag(u8"λ��##" + ActorName + u8"_Scene", pTransform->Translate);
                    glm::vec3 Euler = pTransform->Rotate.getEulerDegrees();
                    UI::drag(u8"��ת##" + ActorName + u8"_Scene", Euler, 0.1f);
                    pTransform->Rotate.setEulerDegrees(Euler);
                    UI::drag(u8"����##" + ActorName + u8"_Scene", pTransform->Scale);
                    
                    UI::toggle(u8"Ϊ��̬����##" + ActorName + u8"_Scene", pActor->getPhysicsState()->IsStatic);
                    UI::toggle(u8"��������##" + ActorName + u8"_Scene", pActor->getPhysicsState()->HasGravity);
                    UI::drag(u8"����##" + ActorName + u8"_Scene", pActor->getPhysicsState()->Mass, 0.01f, 0.01f);
                    UI::drag(u8"�ٶ�##" + ActorName + u8"_Scene", pActor->getPhysicsState()->Velocity);
                    UI::drag(u8"���ٶ�##" + ActorName + u8"_Scene", pActor->getPhysicsState()->AngularVelocity);
                    UI::unindent();
                }
            }
            UI::unindent();
        }
    }
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> ShadeBuffers = m_pPassShade->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = ShadeBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CGUIRenderPass>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    m_pPassShade = make<CRenderPassShade>();
    m_pPassShade->init(AppInfo);
    m_pPassShade->setCamera(m_pCamera);
    
    m_pScene = __generateScene();
    __resetActors();
    __initPhysicsEngine();
    m_pPassShade->setScene(m_pScene);
    
    __linkPasses();
}

void CApplicationTest::_recreateOtherResourceV()
{
}

void CApplicationTest::_destroyOtherResourceV()
{
    destroyAndClear(m_pPassGUI);
    destroyAndClear(m_pPassShade);

    cleanGlobalCommandBuffer();
}

void CApplicationTest::__initPhysicsEngine()
{
    for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
    {
        auto pActor = m_pScene->getActor(i);
        m_pPhysicsEngine->add(pActor->getPhysicsState());
    }
}

void CApplicationTest::__resetActors()
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

void CApplicationTest::__linkPasses()
{
    auto pPortShade = m_pPassShade->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortShade, "Main");;
    CPortSet::link(pPortShade,  "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
}