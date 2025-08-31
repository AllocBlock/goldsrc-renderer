#include "ApplicationVisualize.h"

#include "ImageUtils.h"
#include "SingleTimeCommandBuffer.h"

void CApplicationVisualize::addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor)
{ m_pPassVisualize->addTriangle(vTriangle, vColor); }

void CApplicationVisualize::addLine(const Visualize::Line& vLine, const glm::vec3& vColor)
{ m_pPassVisualize->addLine(vLine, vColor); }

void CApplicationVisualize::addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor)
{ m_pPassVisualize->addPoint(vPoint, vColor); }

void CApplicationVisualize::addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{ m_pPassVisualize->addSphere(vCenter, vScale, vColor); }

void CApplicationVisualize::addCube(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor)
{ m_pPassVisualize->addCube(vCenter, vScale, vColor); }

void CApplicationVisualize::_createV()
{
    SingleTimeCommandBuffer::setup(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setPos(glm::vec3(1, 1, 1));
    pCamera->setAt(glm::vec3(0, 0, 0));

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, pCamera);

    VkExtent2D ScreenExtent = m_pSwapchain->getExtent();
    m_pPassVisualize = make<CRenderPassVisualize>();
    m_pPassVisualize->createPortSet();
    m_pPassPresent = make<CRenderPassPresent>(m_pSwapchain);
    m_pPassPresent->createPortSet();

    __linkPasses();

    m_pPassVisualize->init(m_pDevice, ScreenExtent);
    m_pPassVisualize->setSceneInfo(m_pSceneInfo);
    m_pPassPresent->init(m_pDevice, ScreenExtent);
}

void CApplicationVisualize::_updateV(uint32_t ImageIndex)
{
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setAspect(vk::calcAspect(m_pSwapchain->getExtent()));
    m_pInteractor->update();
    m_pPassVisualize->update();
    m_pPassPresent->updateSwapchainImageIndex(ImageIndex);
    m_pPassPresent->update();
}

std::vector<VkCommandBuffer> CApplicationVisualize::_getCommandBuffers()
{
    auto buffers = std::vector<VkCommandBuffer>();
    const auto& subBuffers1 = m_pPassVisualize->requestCommandBuffers();
    buffers.insert(buffers.end(), subBuffers1.begin(), subBuffers1.end());
    const auto& subBuffers2 = m_pPassPresent->requestCommandBuffers();
    buffers.insert(buffers.end(), subBuffers2.begin(), subBuffers2.end());
    return buffers;
}

void CApplicationVisualize::_destroyV()
{
    destroyAndClear(m_pPassVisualize);
    destroyAndClear(m_pPassPresent);
    SingleTimeCommandBuffer::clean();
}

void CApplicationVisualize::__linkPasses()
{
    auto pPortVisualize = m_pPassVisualize->getPortSet();
    auto pPortPresent = m_pPassPresent->getPortSet();

    CPortSet::link(pPortVisualize, "Main", pPortPresent, "Main");
}
