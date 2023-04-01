#include "ApplicationSprite.h"

#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationSprite::_createV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();

    m_pCamera = make<CCamera>();
    m_pCamera->setFov(90);
    m_pCamera->setAspect(ScreenExtent.width, ScreenExtent.height);
    m_pCamera->setPos(glm::vec3(1.0, 0.0, 0.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));

    m_pPassSprite = make<CRenderPassSprite>();
    m_pPassSprite->init(m_pDevice, m_pAppInfo);
    m_pPassSprite->setCamera(m_pCamera);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(m_pDevice, m_pAppInfo);

    __linkPasses();
}

void CApplicationSprite::_updateV(uint32_t vImageIndex)
{
    m_pCamera->setAspect(m_pAppInfo->getScreenAspect());
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassSprite->update(vImageIndex);
}

void CApplicationSprite::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"SpräÖÈ¾");
    UI::text(u8"²âÊÔ");
    m_pCamera->renderUI();
    UI::endWindow();
    UI::endFrame();
}

void CApplicationSprite::_destroyV()
{
    destroyAndClear(m_pPassSprite);
    destroyAndClear(m_pPassGUI);
    m_pInteractor = nullptr;

    cleanGlobalCommandBuffer();
}

void CApplicationSprite::__linkPasses()
{
    auto pPortMain = m_pPassSprite->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        CPortSet::link(m_pSwapchainPort, pPortMain, "Main");
        CPortSet::link(pPortMain, "Main", pPortGui, "Main");
    }
    m_pSwapchainPort->setForceNotReady(false);

    _ASSERTE(m_pPassSprite->isValid());
    _ASSERTE(m_pPassGUI->isValid());
}
