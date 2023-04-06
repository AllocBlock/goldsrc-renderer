#include "ApplicationGoldSrc.h"
#include "PassGoldSrc.h"
#include "SceneProbe.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

void CApplicationGoldSrc::_createV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    m_pCamera = make<CCamera>();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(m_pDevice, m_pAppInfo);

    m_pPassOutlineMask = make<CRenderPassOutlineMask>();
    m_pPassOutlineMask->init(m_pDevice, m_pAppInfo);
    m_pPassOutlineMask->setCamera(m_pCamera);

    m_pPassOutlineEdge = make<CRenderPassOutlineEdge>();
    m_pPassOutlineEdge->init(m_pDevice, m_pAppInfo);

    m_pPassVisualize = make<CRenderPassVisualize>();
    m_pPassVisualize->init(m_pDevice, m_pAppInfo);
    m_pPassVisualize->setCamera(m_pCamera);

    m_pMainUI = make<CGUIMain>();
    m_pMainUI->setInteractor(m_pInteractor);

    m_pMainUI->setReadSceneCallback([this](ptr<SSceneInfoGoldSrc> vScene)
        {
            m_pSceneInfo = vScene;
            m_pPassGoldSrc->loadScene(vScene);
        });
    m_pMainUI->setRenderSettingCallback([this]()
        {
            if (m_pCamera) m_pCamera->renderUI();
            if (m_pInteractor) m_pInteractor->renderUI();
            if (m_pPassOutlineMask) m_pPassOutlineMask->renderUI();
            if (m_pPassOutlineEdge) m_pPassOutlineEdge->renderUI();
            if (m_pPassGoldSrc) m_pPassGoldSrc->renderUI();
        });

    m_pInteractor->setMouseCallback([this](GLFWwindow* vWindow, int vButton, int vAction)
        {
            if (!m_pSceneInfo || !m_pSceneInfo->pScene) return;
            if (vButton != GLFW_MOUSE_BUTTON_LEFT || vAction != GLFW_RELEASE) return;

            double XPos = 0.0, YPos = 0.0;
            glfwGetCursorPos(vWindow, &XPos, &YPos);
            int WindowWidth = 0, WindowHeight = 0;
            glfwGetFramebufferSize(vWindow, &WindowWidth, &WindowHeight);
            glm::vec2 NDC = glm::vec2(XPos / WindowWidth * 2 - 1.0, YPos / WindowHeight * 2 - 1.0);

            CActor::Ptr pNearestActor = nullptr;
            glm::vec3 NearestIntersection;
            if (SceneProbe::select(NDC, m_pCamera, m_pSceneInfo->pScene, pNearestActor, NearestIntersection))
            {
                m_pPassOutlineMask->setHighlightActor(pNearestActor);
                m_pPassVisualize->addLine(m_pCamera->getPos(), NearestIntersection, glm::vec3(0.0, 1.0, 0.0));
                m_pMainUI->setSceneFocusedActor(pNearestActor);
            }
            else
            {
                m_pPassOutlineMask->removeHighlight();
                m_pMainUI->clearSceneFocusedActor();
            }
        });

    m_pPassGoldSrc = make<CRenderPassGoldSrc>();
    m_pPassGoldSrc->init(m_pDevice, m_pAppInfo);
    m_pPassGoldSrc->setCamera(m_pCamera);
    if (m_pSceneInfo)
        m_pPassGoldSrc->loadScene(m_pSceneInfo);

    __linkPasses();
}

void CApplicationGoldSrc::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassGoldSrc->update(vImageIndex);
    m_pPassOutlineMask->update(vImageIndex);
    m_pPassOutlineEdge->update(vImageIndex);
    m_pPassVisualize->update(vImageIndex);
}

void CApplicationGoldSrc::_renderUIV()
{
    UI::beginFrame();
    m_pMainUI->renderUI();
    UI::endFrame();
}

void CApplicationGoldSrc::_destroyV()
{
    destroyAndClear(m_pPassGoldSrc);
    destroyAndClear(m_pPassOutlineMask);
    destroyAndClear(m_pPassOutlineEdge);
    destroyAndClear(m_pPassGUI);
    destroyAndClear(m_pPassVisualize);

    cleanGlobalCommandBuffer();
}

void CApplicationGoldSrc::__linkPasses()
{
    auto pPortScene = m_pPassGoldSrc->getPortSet();
    auto pPortOutlineMask = m_pPassOutlineMask->getPortSet();
    auto pPortOutlineEdge = m_pPassOutlineEdge->getPortSet();
    auto pPortVisualize = m_pPassVisualize->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();
    m_pSwapchainPort->unlinkAll();
    pPortScene->unlinkAll();
    pPortOutlineMask->unlinkAll();
    pPortOutlineEdge->unlinkAll();
    pPortVisualize->unlinkAll();
    pPortGui->unlinkAll();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortScene, "Main");
    CPortSet::link(pPortOutlineMask, "Mask", pPortOutlineEdge, "Mask");
    CPortSet::link(pPortScene, "Main", pPortOutlineEdge, "Main");
    CPortSet::link(pPortOutlineEdge, "Main", pPortVisualize, "Main");
    CPortSet::link(pPortVisualize, "Main", pPortGui, "Main");
    CPortSet::link(pPortScene, "Depth", pPortVisualize, "Depth");
    m_pSwapchainPort->setForceNotReady(false);

    _ASSERTE(m_pPassGoldSrc->isValid());
    _ASSERTE(m_pPassOutlineMask->isValid());
    _ASSERTE(m_pPassOutlineEdge->isValid());
    _ASSERTE(m_pPassVisualize->isValid());
    _ASSERTE(m_pPassGUI->isValid());
}
