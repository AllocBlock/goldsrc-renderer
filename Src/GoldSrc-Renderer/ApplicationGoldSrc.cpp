#include "ApplicationGoldSrc.h"
#include "ApplicationGoldSrc.h"
#include "PassGoldSrc.h"
#include "PassSimple.h"
#include "Common.h"
#include "SceneProbe.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

#include <iostream>
#include <set>

void CApplicationGoldSrc::_initV()
{
    m_pCamera = make<CCamera>();

    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());
}

void CApplicationGoldSrc::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassScene->update(vImageIndex);
    m_pPassOutlineMask->update(vImageIndex);
    m_pPassOutlineEdge->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationGoldSrc::_getCommandBufferSetV(uint32_t vImageIndex)
{
    // FIXME: auto process
    std::vector<VkCommandBuffer> SceneBuffers = m_pPassScene->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> OutlineMaskBuffers = m_pPassOutlineMask->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> OutlineEdgeBuffers = m_pPassOutlineEdge->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), OutlineMaskBuffers.begin(), OutlineMaskBuffers.end());
    Result.insert(Result.end(), OutlineEdgeBuffers.begin(), OutlineEdgeBuffers.end());
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationGoldSrc::_renderUIV()
{
    UI::beginFrame();
    m_pMainUI->renderUI();
    UI::endFrame();
}

void CApplicationGoldSrc::_createOtherResourceV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CGUIRenderPass>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    m_pPassOutlineMask = make<COutlineMaskRenderPass>();
    m_pPassOutlineMask->init(AppInfo);
    m_pPassOutlineMask->setCamera(m_pCamera);

    m_pPassOutlineEdge = make<COutlineEdgeRenderPass>();
    m_pPassOutlineEdge->init(AppInfo);

    m_pMainUI = make<CGUIMain>();
    m_pMainUI->setInteractor(m_pInteractor);
    m_pMainUI->setChangeRendererCallback([this](ERenderMethod vMethod)
    {
        __recreateRenderer(vMethod);
    });

    m_pMainUI->setReadSceneCallback([this](ptr<SScene> vScene)
    {
        m_pScene = vScene;
        m_pPassScene->loadScene(vScene);
    });
    m_pMainUI->setRenderSettingCallback([this]()
    {
        if (m_pCamera) m_pCamera->renderUI();
        if (m_pInteractor) m_pInteractor->renderUI();
        if (m_pPassOutlineMask) m_pPassOutlineMask->renderUI();
        if (m_pPassOutlineEdge) m_pPassOutlineEdge->renderUI();
        if (m_pPassScene) m_pPassScene->renderUI();
    });

    m_pInteractor->setMouseCallback([this](GLFWwindow* vWindow, int vButton, int vAction) 
    {
        double XPos = 0.0, YPos = 0.0;
        glfwGetCursorPos(vWindow, &XPos, &YPos);
        int WindowWidth = 0, WindowHeight = 0;
        glfwGetFramebufferSize(vWindow, &WindowWidth, &WindowHeight);
        glm::vec2 NDC = glm::vec2(XPos / WindowWidth * 2 - 1.0, YPos / WindowHeight * 2 - 1.0);

        ptr<CMeshDataGoldSrc> pNearestObject = nullptr;
        float NearestIntersection = 0.0f;
        if (SceneProbe::select(NDC, m_pCamera, m_pScene, pNearestObject, NearestIntersection))
        {
            m_pPassOutlineMask->setHighlightObject(pNearestObject);
        }
    });

    __recreateRenderer(ERenderMethod::BSP);

    _recreateOtherResourceV();
}

void CApplicationGoldSrc::_recreateOtherResourceV()
{
    m_pPassScene->updateImageInfo(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pPassOutlineMask->updateImageInfo(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pPassOutlineEdge->updateImageInfo(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pPassGUI->updateImageInfo(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
}

void CApplicationGoldSrc::_destroyOtherResourceV()
{
    destroyAndClear(m_pPassScene);
    destroyAndClear(m_pPassOutlineMask);
    destroyAndClear(m_pPassOutlineEdge);
    destroyAndClear(m_pPassGUI);

    cleanGlobalCommandBuffer();
}

void CApplicationGoldSrc::__recreateRenderer(ERenderMethod vMethod)
{
    m_pDevice->waitUntilIdle();
    if (m_pPassScene)
        m_pPassScene->destroy();

    auto AppInfo = getAppInfo();
    switch (vMethod)
    {
    case ERenderMethod::DEFAULT:
    {
        m_pPassScene = make<CSceneSimpleRenderPass>();
        break;
    }
    case ERenderMethod::BSP:
    {
        m_pPassScene = make<CSceneGoldSrcRenderPass>();
        break;
    }
    default:
        return;
    }

    m_pPassScene->init(AppInfo);
    m_pPassScene->setCamera(m_pCamera);
    if (m_pScene)
        m_pPassScene->loadScene(m_pScene);

    __linkPasses();
}

void CApplicationGoldSrc::__linkPasses()
{
    auto pPortScene = m_pPassScene->getPortSet();
    auto pPortOutlineMask = m_pPassOutlineMask->getPortSet();
    auto pPortOutlineEdge = m_pPassOutlineEdge->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortScene, "Main");
    CPortSet::link(pPortScene, "Depth", pPortOutlineMask, "Depth");
    CPortSet::link(pPortOutlineMask, "Mask", pPortOutlineEdge, "Mask");
    CPortSet::link(pPortScene, "Main", pPortOutlineEdge, "Main");
    CPortSet::link(pPortOutlineEdge, "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);

    _ASSERTE(m_pPassScene->isValid());
    _ASSERTE(m_pPassOutlineMask->isValid());
    _ASSERTE(m_pPassOutlineEdge->isValid());
    _ASSERTE(m_pPassGUI->isValid());
}