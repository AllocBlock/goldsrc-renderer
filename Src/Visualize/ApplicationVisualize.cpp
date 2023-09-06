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

    m_pPassVisualize = make<CRenderPassVisualize>();
    m_pPassVisualize->init(m_pDevice, m_pAppInfo);
    m_pPassVisualize->setSceneInfo(m_pSceneInfo);

    SPortFormat Format = { VK_FORMAT_D32_SFLOAT, SPortFormat::AnyExtent, 1, EUsage::WRITE };
    m_pDepthPort = make<CSourcePort>("Depth", Format, nullptr);

    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();
    VkFormat DepthFormat = m_pDepthPort->getFormat().Format;
    ImageUtils::createDepthImage(m_DepthImage, m_pDevice, ScreenExtent, NULL, DepthFormat);

    m_pDepthPort->setActualFormat(DepthFormat);
    m_pDepthPort->setActualExtent(ScreenExtent);
    m_pDepthPort->setImage(m_DepthImage, 0);

    __linkPasses();
}

void CApplicationVisualize::_updateV(uint32_t vImageIndex)
{
    CCamera::Ptr pCamera = m_pSceneInfo->pScene->getMainCamera();
    pCamera->setAspect(m_pAppInfo->getScreenAspect());
    m_pInteractor->update();
    m_pPassVisualize->update(vImageIndex);
}

void CApplicationVisualize::_destroyV()
{
    m_DepthImage.destroy();
    destroyAndClear(m_pPassVisualize);
    SingleTimeCommandBuffer::clean();
}

void CApplicationVisualize::__linkPasses()
{
    auto pPortVisualize = m_pPassVisualize->getPortSet();
    m_pSwapchainPort->unlinkAll();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortVisualize, "Main");
    CPortSet::link(m_pDepthPort, pPortVisualize, "Depth");
    m_pSwapchainPort->setForceNotReady(false);

    _ASSERTE(m_pPassVisualize->isValid());
}
