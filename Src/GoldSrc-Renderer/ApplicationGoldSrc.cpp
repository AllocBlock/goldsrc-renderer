#include "ApplicationGoldSrc.h"
#include "ApplicationGoldSrc.h"
#include "PassGoldSrc.h"
#include "PassSimple.h"
#include "Common.h"
#include "SceneProbe.h"
#include "GlobalSingleTimeBuffer.h"
#include "Gui.h"

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
    m_pPassLine->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationGoldSrc::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pPassScene->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> LineBuffers = m_pPassLine->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), LineBuffers.begin(), LineBuffers.end());
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
    m_pPassGUI->init(AppInfo, vk::ERenderPassPos::END);

    m_pPassLine = make<CLineRenderPass>();
    m_pPassLine->init(AppInfo, vk::ERenderPassPos::MIDDLE);
    m_pPassLine->setCamera(m_pCamera);

    m_pMainUI = make<CGUIMain>();
    m_pMainUI->setInteractor(m_pInteractor);
    m_pMainUI->setChangeRendererCallback([this](ERenderMethod vMethod)
    {
        __recreateRenderer(vMethod);
        __linkPasses();
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
        if (m_pPassLine) m_pPassLine->renderUI();
        if (m_pPassScene) m_pPassScene->renderUI();
    });

    m_pInteractor->setMouseCallback([this](GLFWwindow* vWindow, int vButton, int vAction) 
    {
        double XPos = 0.0, YPos = 0.0;
        glfwGetCursorPos(vWindow, &XPos, &YPos);
        int WindowWidth = 0, WindowHeight = 0;
        glfwGetFramebufferSize(vWindow, &WindowWidth, &WindowHeight);
        glm::vec2 NDC = glm::vec2(XPos / WindowWidth * 2 - 1.0, YPos / WindowHeight * 2 - 1.0);

        float NearestIntersection = 0.0f;
        S3DBoundingBox BB;
        if (SceneProbe::select(NDC, m_pCamera, m_pScene, NearestIntersection, BB))
        {
            m_pPassLine->setHighlightBoundingBox(BB);
            m_pPassLine->addGuiLine("Direction", m_pCamera->getPos(), m_pCamera->getPos() + m_pCamera->getFront());
        }
    });

    _recreateOtherResourceV();
}

void CApplicationGoldSrc::_recreateOtherResourceV()
{
    __recreateRenderer(ERenderMethod::BSP);
    m_pPassLine->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pPassGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    __linkPasses();
}

void CApplicationGoldSrc::_destroyOtherResourceV()
{
    m_pPassScene->destroy();
    m_pPassLine->destroy();
    m_pPassGUI->destroy();

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

    m_pPassScene->init(AppInfo, vk::ERenderPassPos::BEGIN);
    m_pPassScene->setCamera(m_pCamera);
    if (m_pScene)
        m_pPassScene->loadScene(m_pScene);
}

void CApplicationGoldSrc::__linkPasses()
{
    auto pLinkScene = m_pPassScene->getLink();
    auto pLinkLine = m_pPassLine->getLink();
    auto pLinkGui = m_pPassGUI->getLink();

    const auto& ImageViews = m_pSwapchain->getImageViews();
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        pLinkScene->linkOutput("Output", ImageViews[i], i);
        pLinkLine->linkInput("Input", ImageViews[i], i);
        pLinkLine->linkInput("Depth", pLinkScene->getOutput("Depth", i), i);
        pLinkLine->linkOutput("Output", ImageViews[i], i);
        pLinkGui->linkInput("Input", ImageViews[i], i);
        pLinkGui->linkOutput("Output", ImageViews[i], i);
    }
}