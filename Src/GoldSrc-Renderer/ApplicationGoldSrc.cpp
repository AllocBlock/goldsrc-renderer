#include "ApplicationGoldSrc.h"
#include "SceneProbe.h"
#include "SingleTimeCommandBuffer.h"
#include "InterfaceUI.h"
#include "RenderPassGraph.h"
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
    
    m_pRenderPassGraph->NodeMap[0] = SRenderPassGraphNode("GoldSrc");
    m_pRenderPassGraph->NodeMap[1] = SRenderPassGraphNode("Outline");
    m_pRenderPassGraph->NodeMap[2] = SRenderPassGraphNode("Visualize");
    m_pRenderPassGraph->NodeMap[3] = SRenderPassGraphNode("Gui");
    
    m_pRenderPassGraph->LinkMap[0] = { {0, "Main"}, {1, "Main"} };
    m_pRenderPassGraph->LinkMap[1] = { {1, "Main"}, {2, "Main"} };
    m_pRenderPassGraph->LinkMap[2] = { {2, "Main"}, {3, "Main"} };
    m_pRenderPassGraph->LinkMap[3] = { {0, "Depth"}, {2, "Depth"} };

    m_pRenderPassGraph->EntryPortOpt = { 0, "Main" };

    m_pRenderPassGraphUI = make<CRenderPassGraphUI>();
    m_pRenderPassGraphUI->setContext(m_pDevice, m_pAppInfo);
    m_pRenderPassGraphUI->setGraph(m_pRenderPassGraph);

    m_pGraphInstance->createFromGraph(m_pRenderPassGraph, m_pSwapchainPort, 
        [this](const std::string& vName, vk::IRenderPass::Ptr vRenderPass)
        {
            // GUI pass need to set window
            if (vName == "Gui")
            {
                ptr<CRenderPassGUI> pPassGUI = std::dynamic_pointer_cast<CRenderPassGUI>(vRenderPass);
                pPassGUI->setWindow(m_pWindow);
            }
        }
    );
}

void CApplicationGoldSrc::_updateV(uint32_t vImageIndex)
{
    auto pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setAspect(m_pAppInfo->getScreenAspect());
    m_pInteractor->update();
    m_pRenderPassGraphUI->update();
}

void CApplicationGoldSrc::_renderUIV()
{
    UI::beginFrame();
    m_pMainUI->renderUI();
    
    if (UI::beginWindow("Render Pass Graph"))
    {
        m_pRenderPassGraphUI->renderUI();
    }
    UI::endWindow();
    UI::endFrame();
}

void CApplicationGoldSrc::_destroyV()
{
    m_pRenderPassGraphUI->destroyPasses();
    SingleTimeCommandBuffer::clean();
}