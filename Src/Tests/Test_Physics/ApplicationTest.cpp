#include "ApplicationTest.h"
#include "GlobalSingleTimeBuffer.h"
#include "Gui.h"

using namespace vk;

CTempScene::Ptr __generateScene()
{
    CTempScene::Ptr m_pScene = make<CTempScene>();

    // ground
    glm::vec3 Normal = glm::vec3(0.0, 0.0, 1.0);
    std::vector<glm::vec3> VertexSet =
    {
        glm::vec3(10,  10, 0),
        glm::vec3(10, -10, 0),
        glm::vec3(-10, -10, 0),
        glm::vec3(10,  10, 0),
        glm::vec3(-10, -10, 0),
        glm::vec3(-10,  10, 0),
    };

    auto pGroundMesh = make<CMeshTriangleList>();
    pGroundMesh->addTriangles(VertexSet, { Normal, Normal, Normal, Normal, Normal, Normal });

    auto pGroundActor = make<CActor>("Ground");
    pGroundActor->setMesh(pGroundMesh);

    m_pScene->addActor(pGroundActor);

    // cubes
    auto pCubeMesh1 = make<CMeshBasicCube>(glm::vec3(0.0, 0.0, 0.0), 5.0f);
    auto pCubeActor1 = make<CActor>("Cube1");
    pCubeActor1->setMesh(pCubeMesh1);
    m_pScene->addActor(pCubeActor1);


    auto pCubeMesh2 = make<CMeshBasicCube>(glm::vec3(0.0, 3.0, 0.0), 1.0f);
    auto pCubeActor2 = make<CActor>("Cube2");
    pCubeActor2->setMesh(pCubeMesh2);
    m_pScene->addActor(pCubeActor2);

    return m_pScene;
}

void CApplicationTest::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    auto AppInfo = getAppInfo();

    m_pCamera = make<CCamera>();
    m_pCamera->setFov(90);
    m_pCamera->setAspect(AppInfo.Extent.width / AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(10.0, 10.0, 10.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassShade->update(vImageIndex);
}

void CApplicationTest::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"物理系统 Physics");
    UI::text(u8"测试");
    m_pInteractor->getCamera()->renderUI();

    // scene
    {
        if (UI::collapse(u8"场景", true))
        {
            UI::indent(20.0f);
            for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
            {
                auto pActor = m_pScene->getActor(i);
                std::string ActorName = pActor->getName();
                if (UI::collapse(ActorName + "###Scene"))
                {
                    UI::indent(20.0f);
                    auto Transform = pActor->getTransform();
                    UI::drag(u8"位置###" + ActorName + u8"_scene", Transform.Translate);
                    UI::drag(u8"旋转###" + ActorName + u8"_scene", Transform.Rotate);
                    UI::drag(u8"缩放###" + ActorName + u8"_scene", Transform.Scale);
                    pActor->setTransform(Transform);
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

void CApplicationTest::__linkPasses()
{
    auto pPortShade = m_pPassShade->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortShade, "Main");;
    CPortSet::link(pPortShade,  "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
}