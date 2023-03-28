#include "ApplicationVisualize.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

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

    __linkPasses();
}

void CApplicationVisualize::_updateV(uint32_t vImageIndex)
{
    m_pCamera->setAspect(m_pAppInfo->getScreenAspect());
    m_pInteractor->update();
    m_pPassVisualize->update(vImageIndex);
}

void CApplicationVisualize::_destroyV()
{
    destroyAndClear(m_pPassVisualize);
    cleanGlobalCommandBuffer();
}

void CApplicationVisualize::__linkPasses()
{
    auto pPortVisualize = m_pPassVisualize->getPortSet();
    m_pSwapchainPort->unlinkAll();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortVisualize, "Main");
    m_pSwapchainPort->setForceNotReady(false);

    _ASSERTE(m_pPassVisualize->isValid());
}
