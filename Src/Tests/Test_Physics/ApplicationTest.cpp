#include "ApplicationTest.h"

#include <random>

#include "GlobalSingleTimeBuffer.h"
#include "Gui.h"
#include "Ticker.h"

using namespace vk;

CTempScene::Ptr __generateScene()
{
    CTempScene::Ptr m_pScene = make<CTempScene>();

    // ground
    glm::vec3 Normal = glm::vec3(0.0, 0.0, 1.0);
    std::vector<glm::vec3> VertexSet =
    {
        glm::vec3(20,  20, 0),
        glm::vec3(20, -20, 0),
        glm::vec3(-20, -20, 0),
        glm::vec3(20,  20, 0),
        glm::vec3(-20, -20, 0),
        glm::vec3(-20,  20, 0),
    };

    auto pGroundMesh = make<CMeshTriangleList>();
    pGroundMesh->addTriangles(VertexSet, { Normal, Normal, Normal, Normal, Normal, Normal });

    auto pGroundActor = make<CActor>("Ground");
    pGroundActor->setMesh(pGroundMesh);
    pGroundActor->getPhysicsState()->IsStatic = true;
    pGroundActor->getPhysicsState()->pCollider = make<CColliderBasic>(pGroundActor->getTransform(), EBasicColliderType::PLANE);

    m_pScene->addActor(pGroundActor);

    //// cubes
    //auto pCubeMesh1 = make<CMeshBasicCube>();
    //auto pCubeActor1 = make<CActor>("Cube1");
    //pCubeActor1->setMesh(pCubeMesh1);
    //pCubeActor1->getPhysicsState()->pCollider = make<CColliderBasic>(pCubeActor1->getTransform(), EBasicColliderType::CUBE);
    //m_pScene->addActor(pCubeActor1);
    //
    //auto pCubeMesh2 = make<CMeshBasicCube>();
    //auto pCubeActor2 = make<CActor>("Cube2");
    //pCubeActor2->setMesh(pCubeMesh2);
    //pCubeActor2->getPhysicsState()->pCollider = make<CColliderBasic>(pCubeActor2->getTransform(), EBasicColliderType::CUBE);
    //m_pScene->addActor(pCubeActor2);

    // Sphere
    auto pCubeMesh1 = make<CMeshBasicCube>();
    auto pCubeActor1 = make<CActor>("Cube1");
    pCubeActor1->setMesh(pCubeMesh1);
    pCubeActor1->getPhysicsState()->pCollider = make<CColliderBasic>(pCubeActor1->getTransform(), EBasicColliderType::SPHERE);
    m_pScene->addActor(pCubeActor1);

    auto pCubeMesh2 = make<CMeshBasicCube>();
    auto pCubeActor2 = make<CActor>("Cube2");
    pCubeActor2->setMesh(pCubeMesh2);
    pCubeActor2->getPhysicsState()->pCollider = make<CColliderBasic>(pCubeActor2->getTransform(), EBasicColliderType::SPHERE);
    m_pScene->addActor(pCubeActor2);

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
    UI::beginWindow(u8"物理系统 Physics");
    UI::text(u8"测试");
    m_pInteractor->getCamera()->renderUI();

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
                auto pCube1 = m_pScene->findActor("Cube1");
                pCube1->getPhysicsState()->addForce(__generateRandomUpForce());
                auto pCube2 = m_pScene->findActor("Cube2");
                pCube2->getPhysicsState()->addForce(__generateRandomUpForce());
            }

            if (UI::button(u8"随机角加速度"))
            {
                auto pCube1 = m_pScene->findActor("Cube1");
                pCube1->getPhysicsState()->addAlpha(__generateRandomAlpha());
                auto pCube2 = m_pScene->findActor("Cube2");
                pCube2->getPhysicsState()->addAlpha(__generateRandomAlpha());
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
    auto pCube1 = m_pScene->findActor("Cube1");
    pCube1->resetTransform();
    pCube1->setTranslate(glm::vec3(0.0f, 0.0f, 16.0f));
    pCube1->setScale(3.0f);
    pCube1->clearMoveState();
    auto pCube2 = m_pScene->findActor("Cube2");
    pCube2->resetTransform();
    pCube2->setTranslate(glm::vec3(0.0f, 6.0f, 20.0f));
    pCube2->clearMoveState();
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