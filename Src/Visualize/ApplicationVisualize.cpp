#include "ApplicationVisualize.h"

#include "Function.h"
#include "GlobalSingleTimeBuffer.h"

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
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    m_pCamera = make<CCamera>();
    m_pCamera->setPos(glm::vec3(1, 1, 1));
    m_pCamera->setAt(glm::vec3(0, 0, 0));

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassVisualize = make<CRenderPassVisualize>();
    m_pPassVisualize->init(m_pDevice, m_pAppInfo);
    m_pPassVisualize->setCamera(m_pCamera);

    SPortFormat Format = { VK_FORMAT_D32_SFLOAT, SPortFormat::AnyExtent, 1, EUsage::WRITE };
    m_pDepthPort = make<CSourcePort>("Depth", Format, nullptr);

    m_DepthImageManager.init({0, 0}, true,
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            vImageSet.init(1);
            VkFormat DepthFormat = m_pDepthPort->getFormat().Format;
            EUsage Usage = m_pDepthPort->getFormat().Usage;
            Function::createDepthImage(*vImageSet[0], m_pDevice, vExtent, NULL, DepthFormat);

            /*VkCommandBuffer CommandBuffer = vk::beginSingleTimeBuffer();
            vImageSet[0]->transitionLayout(CommandBuffer, toLayout(Usage, true));
            vk::endSingleTimeBuffer(CommandBuffer);*/

            m_pDepthPort->setActualFormat(DepthFormat);
            m_pDepthPort->setActualExtent(vExtent);
            m_pDepthPort->setImage(*vImageSet[0], 0);
        }
    );

    __linkPasses();

    // create after link
    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();
    m_DepthImageManager.updateExtent(ScreenExtent, true);
}

void CApplicationVisualize::_updateV(uint32_t vImageIndex)
{
    m_pCamera->setAspect(m_pAppInfo->getScreenAspect());
    m_pInteractor->update();
    m_pPassVisualize->update(vImageIndex);
}

void CApplicationVisualize::_destroyV()
{
    m_DepthImageManager.destroy();
    destroyAndClear(m_pPassVisualize);
    cleanGlobalCommandBuffer();
}

void CApplicationVisualize::_onResizeV()
{
    m_DepthImageManager.updateExtent(m_pAppInfo->getScreenExtent());
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
