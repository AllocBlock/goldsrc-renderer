#include "ApplicationSprite.h"

#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationSprite::_createV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    vk::SAppInfo AppInfo = getAppInfo();

    m_pPassMain = make<CRenderPassSprite>();
    m_pPassMain->init(AppInfo);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pPassMain->getCamera());

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    __linkPasses();
}

void CApplicationSprite::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassMain->update(vImageIndex);
}

void CApplicationSprite::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"SpräÖÈ¾");
    UI::text(u8"²âÊÔ");
    m_pInteractor->getCamera()->renderUI();
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationSprite::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> BufferSet;
    std::vector<VkCommandBuffer> RendererBufferSet = m_pPassMain->requestCommandBuffers(vImageIndex);
    BufferSet.insert(BufferSet.end(), RendererBufferSet.begin(), RendererBufferSet.end());
    std::vector<VkCommandBuffer> GUIBufferSet = m_pPassGUI->requestCommandBuffers(vImageIndex);
    BufferSet.insert(BufferSet.end(), GUIBufferSet.begin(), GUIBufferSet.end());
    return BufferSet;
}

void CApplicationSprite::_destroyV()
{
    destroyAndClear(m_pPassMain);
    destroyAndClear(m_pPassGUI);
    m_pInteractor = nullptr;

    cleanGlobalCommandBuffer();
}

void CApplicationSprite::__linkPasses()
{
    auto pPortMain = m_pPassMain->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        CPortSet::link(m_pSwapchainPort, pPortMain, "Main");
        CPortSet::link(pPortMain, "Main", pPortGui, "Main");
    }
    m_pSwapchainPort->setForceNotReady(false);
}
