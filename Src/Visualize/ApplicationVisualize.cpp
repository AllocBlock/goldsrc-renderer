#include "ApplicationVisualize.h"

#include "ImageUtils.h"
#include "SingleTimeCommandBuffer.h"
#include "../Gui/InterfaceGui.h"

void CApplicationVisualize::addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
{
    m_pPassVisualize->addTriangle(vTriangle, vColor);
}

void CApplicationVisualize::addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
{
    m_pPassVisualize->addLine(vLine, vColor);
}

void CApplicationVisualize::addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
{
    m_pPassVisualize->addPoint(vPoint, vColor);
}

void CApplicationVisualize::addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_pPassVisualize->addSphere(vCenter, vScale, vColor);
}

void CApplicationVisualize::addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{
    m_pPassVisualize->addCube(vCenter, vScale, vColor);
}

void CApplicationVisualize::_createV()
{
    SingleTimeCommandBuffer::setup(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    sptr<CCamera> pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setPos(glm::vec3(1, 1, 1));
    pCamera->setAt(glm::vec3(0, 0, 0));

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, pCamera);

    VkExtent2D ScreenExtent = m_pSwapchain->getExtent();
    m_pPassVisualize = make<CRenderPassVisualize>();
    m_pPassVisualize->createPortSet();
    m_pPassGui = make<CRenderPassGUI>(m_pWindow);
    m_pPassGui->createPortSet();
    m_pPassPresent = make<CRenderPassPresent>(m_pSwapchain);
    m_pPassPresent->createPortSet();

    __linkPasses();

    m_pPassVisualize->init(m_pDevice, ScreenExtent);
    m_pPassVisualize->setSceneInfo(m_pSceneInfo);
    m_pPassGui->init(m_pDevice, ScreenExtent);
    m_pPassPresent->init(m_pDevice, ScreenExtent);
}

void CApplicationVisualize::_updateV(uint32_t ImageIndex)
{
    sptr<CCamera> pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setAspect(vk::calcAspect(m_pSwapchain->getExtent()));
    m_pInteractor->update();
    m_pPassVisualize->update();
    m_pPassGui->update();
    m_pPassPresent->updateSwapchainImageIndex(ImageIndex);
    m_pPassPresent->update();
}

void CApplicationVisualize::_renderUIV()
{
    UI::beginFrame();
    std::vector<sptr<engine::IRenderPass>> Passes = { m_pPassVisualize, m_pPassGui, m_pPassPresent };
    for (const auto& pPass : Passes)
    {
        pPass->renderUI();
    }
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationVisualize::_getCommandBuffers()
{
    std::vector<sptr<engine::IRenderPass>> Passes = { m_pPassVisualize, m_pPassGui, m_pPassPresent };
    auto buffers = std::vector<VkCommandBuffer>();
    for (const auto& pPass : Passes)
    {
        const auto& subBuffers = pPass->requestCommandBuffers();
        buffers.insert(buffers.end(), subBuffers.begin(), subBuffers.end());
    }
    return buffers;
}

void CApplicationVisualize::_destroyV()
{
    destroyAndClear(m_pPassVisualize);
    destroyAndClear(m_pPassGui);
    destroyAndClear(m_pPassPresent);
    SingleTimeCommandBuffer::clean();
}

void CApplicationVisualize::__linkPasses()
{
    auto pPortVisualize = m_pPassVisualize->getPortSet();
    auto pPortGui = m_pPassGui->getPortSet();
    auto pPortPresent = m_pPassPresent->getPortSet();

    CPortSet::link(pPortVisualize, "Main", pPortGui, "Main");
    CPortSet::link(pPortGui, "Main", pPortPresent, "Main");
}
