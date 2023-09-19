#include "ApplicationGoldSrc.h"
#include "Environment.h"
#include "RenderPassGraphIO.h"
#include "SceneProbe.h"
#include "SingleTimeCommandBuffer.h"
#include "InterfaceGui.h"
#include "PassGUI.h"
#include "PassOutline.h"
#include "PassPresent.h"
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

    const auto& GraphFile = Environment::findGraph("GoldSrc.graph");
    m_pRenderPassGraph = RenderPassGraphIO::load(GraphFile);
    
    m_pRenderPassGraphUI = make<CRenderPassGraphUI>();
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
            // GUI pass need to set window
            else if (vName == "Present")
            {
                ptr<CRenderPassPresent> pPassPresent = std::dynamic_pointer_cast<CRenderPassPresent>(vRenderPass);
                pPassPresent->setSwapchainPort(m_pSwapchainPort);
            }
        };

        auto pGraph = make<SRenderPassGraph>(*m_pRenderPassGraph);
        // add gui and present pass
        _ASSERTE(pGraph->OutputPort.has_value());
        size_t CurNodeId = 0;
        for (const auto& Pair : pGraph->NodeMap)
        {
            CurNodeId = std::max(CurNodeId, Pair.first);
        }
        size_t GuiNodeId = CurNodeId + 1;
        size_t PresentNodeId = CurNodeId + 2;
        pGraph->NodeMap[GuiNodeId] = SRenderPassGraphNode{"Gui"};
        pGraph->NodeMap[PresentNodeId] = SRenderPassGraphNode{"Present"};

        size_t CurLinkId = 0;
        for (const auto& Pair : pGraph->LinkMap)
        {
            CurLinkId = std::max(CurLinkId, Pair.first);
        }

        pGraph->LinkMap[CurLinkId + 1] = SRenderPassGraphLink{
            pGraph->OutputPort.value(),
            SRenderPassGraphPortInfo{GuiNodeId, "Main"},
        };
        pGraph->LinkMap[CurLinkId + 2] = SRenderPassGraphLink{
            SRenderPassGraphPortInfo{GuiNodeId, "Main"},
            SRenderPassGraphPortInfo{PresentNodeId, "Main"},
        };

        m_pDevice->waitUntilIdle();
        m_pGraphInstance->createFromGraph(pGraph, PresentNodeId, pCreateGraphInstanceCallback);
        m_NeedRecreateGraphInstance = false;
    }

    auto pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setAspect(vk::calcAspect(m_pSwapchain->getExtent()));
    m_pInteractor->update();
    m_pRenderPassGraphUI->update();
}

void CApplicationGoldSrc::_renderUIV()
{
    UI::beginFrame();
    m_pMainUI->renderUI();
    
    m_pRenderPassGraphUI->renderUI();
    UI::endFrame();
}

void CApplicationGoldSrc::_destroyV()
{
    m_pRenderPassGraphUI->destroyPasses();
    SingleTimeCommandBuffer::clean();
}

void CApplicationGoldSrc::_onSwapchainRecreateV()
{
    m_NeedRecreateGraphInstance = true;
}
