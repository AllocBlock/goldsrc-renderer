#include "ApplicationGoldSrc.h"
#include "Environment.h"
#include "RenderPassGraphIO.h"
#include "SceneProbe.h"
#include "SingleTimeCommandBuffer.h"
#include "InterfaceUI.h"
#include "PassGUI.h"
#include "PassGoldSrc.h"
#include "PassOutline.h"
#include "PassVisualize.h"

void CApplicationGoldSrc::_createV()
{
    SingleTimeCommandBuffer::setup(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pSceneInfo->pScene->getMainCamera());
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
            CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();

            auto pPassOutline = m_pGraphInstance->findPass<CRenderPassOutline>();
            if (SceneProbe::select(NDC, pCamera, m_pSceneInfo->pScene, pNearestActor, NearestIntersection))
            {
                if (pPassOutline)
                    pPassOutline->setHighlightActor(pNearestActor);

                auto pPassVisualize = m_pGraphInstance->findPass<CRenderPassVisualize>();
                if (pPassVisualize)
                    pPassVisualize->addLine(pCamera->getPos(), NearestIntersection, glm::vec3(0.0, 1.0, 0.0));
                m_pMainUI->setSceneFocusedActor(pNearestActor);
            }
            else
            {
                if (pPassOutline)
                    pPassOutline->removeHighlight();
                m_pMainUI->clearSceneFocusedActor();
            }
        });

    m_pMainUI = make<CGUIMain>();
    m_pMainUI->setSceneInfo(m_pSceneInfo);

    m_pMainUI->setReadSceneCallback([this]()
        {
            m_pGraphInstance->updateSceneInfo(m_pSceneInfo);
        });
    m_pMainUI->setRenderSettingCallback([this]()
        {
            if (m_pSceneInfo) m_pSceneInfo->pScene->getMainCamera()->renderUI();
            if (m_pInteractor) m_pInteractor->renderUI();
            m_pGraphInstance->renderUI();
        });

    auto GraphFile = Environment::findGraph("GoldSrc.graph");
    m_pRenderPassGraph = RenderPassGraphIO::load(GraphFile);

    

    m_pRenderPassGraphUI = make<CRenderPassGraphUI>();
    m_pRenderPassGraphUI->setContext(m_pDevice, m_pAppInfo);
    m_pRenderPassGraphUI->setGraph(m_pRenderPassGraph);
    m_pRenderPassGraphUI->hookGraphApply([this](ptr<SRenderPassGraph> vGraph)
        {
        m_NeedRecreateGraphInstance = true;
        });

    m_NeedRecreateGraphInstance = true;

}

void CApplicationGoldSrc::_updateV(uint32_t vImageIndex)
{
    if (m_NeedRecreateGraphInstance)
    {
        auto pCreateGraphInstanceCallback = [this](const std::string& vName, vk::IRenderPass::Ptr vRenderPass)
        {
            // GUI pass need to set window
            if (vName == "Gui")
            {
                ptr<CRenderPassGUI> pPassGUI = std::dynamic_pointer_cast<CRenderPassGUI>(vRenderPass);
                pPassGUI->setWindow(m_pWindow);
            }
        };

        m_pDevice->waitUntilIdle();
        m_pGraphInstance->createFromGraph(m_pRenderPassGraph, m_pSwapchainPort, pCreateGraphInstanceCallback);
        m_NeedRecreateGraphInstance = false;
    }

    auto pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setAspect(m_pAppInfo->getScreenAspect());
    m_pInteractor->update();
    m_pRenderPassGraphUI->update();
}

void CApplicationGoldSrc::_renderUIV()
{
    UI::beginFrame();
    m_pMainUI->renderUI();
    
    m_pRenderPassGraphUI->renderUI();
    UI::endWindow();
    UI::endFrame();
}

void CApplicationGoldSrc::_destroyV()
{
    m_pRenderPassGraphUI->destroyPasses();
    SingleTimeCommandBuffer::clean();
}