#include "ApplicationShadowMap.h"
#include "SingleTimeCommandBuffer.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationShadowMap::_createV()
{
    SingleTimeCommandBuffer::setup(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    vk::SAppInfo AppInfo = getAppInfo();

    m_pRenderPassShadowMap = make<CRenderPassShadowMap>();
    m_pRenderPassShadowMap->init(AppInfo);

    m_pPassShade = make<CRenderPassShade>();
    m_pPassShade->init(AppInfo);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pPassShade->getCamera());

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    __generateScene();
    m_pPassShade->setScene(m_pScene);
    m_pRenderPassShadowMap->setScene(m_pScene);

    __linkPasses();
}

void CApplicationShadowMap::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pRenderPassShadowMap->update(vImageIndex);
    m_pPassShade->update(vImageIndex);
}

void CApplicationShadowMap::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"ÒõÓ°Ó³Éä Shadow Map");
    UI::text(u8"²âÊÔ");
    m_pInteractor->getCamera()->renderUI();
    if (UI::button(u8"µ¼³öShadowMapÍ¼Æ¬"))
    {
        m_pRenderPassShadowMap->exportShadowMapToFile("shadowmap.ppm");
    }
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationShadowMap::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> ShadowMapBuffers = m_pRenderPassShadowMap->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> ShadeBuffers = m_pPassShade->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = ShadowMapBuffers;
    Result.insert(Result.end(), ShadeBuffers.begin(), ShadeBuffers.end());
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationShadowMap::_destroyV()
{
    m_pPassGUI->destroy();
    m_pRenderPassShadowMap->destroy();
    m_pPassShade->destroy();

    GlobalCommandBuffer::clean();
}

void CApplicationShadowMap::__linkPasses()
{
    auto pPortShadowMap = m_pRenderPassShadowMap->getPortSet();
    auto pPortShade = m_pPassShade->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pPassShade->setShadowMapInfo(m_pRenderPassShadowMap->getLightCamera());

    m_pSwapchainPort->setForceNotReady(true);
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        CPortSet::link(pPortShadowMap, "ShadowMap", pPortShade, "ShadowMap");
        CPortSet::link(m_pSwapchainPort, pPortShade, "Main");
        CPortSet::link(pPortShade, "Main", pPortGui, "Main");
    }
    m_pSwapchainPort->setForceNotReady(false);

}

void CApplicationShadowMap::__generateScene()
{
    m_pScene = make<CScene>();

    // Ground
    {
        auto pGroundMesh = make<CMeshBasicQuad>();
        auto pGroundActor = make<CActor>("Ground");
        pGroundActor->setMesh(pGroundMesh);
        pGroundActor->setScale(10.0f);
        pGroundActor->bakeTransform();

        m_pScene->addActor(pGroundActor);
    }

    // Cube1
    {
        auto pCubeMesh1 = make<CMeshBasicCube>();
        auto pCubeActor1 = make<CActor>("Cube1");
        pCubeActor1->setMesh(pCubeMesh1);
        pCubeActor1->setScale(2.0f);
        pCubeActor1->bakeTransform();

        m_pScene->addActor(pCubeActor1);
    }

    // Cube2
    {
        auto pCubeMesh2 = make<CMeshBasicCube>();
        auto pCubeActor2 = make<CActor>("Cube2");
        pCubeActor2->setMesh(pCubeMesh2);
        pCubeActor2->setTranslate(glm::vec3(0.0, 3.0, 0.0));
        pCubeActor2->bakeTransform();

        m_pScene->addActor(pCubeActor2);
    }
}
