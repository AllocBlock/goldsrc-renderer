#include "ApplicationTest.h"

void CApplicationTest::_initV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pGUI = make<CGUITest>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, ERenderPassPos::END);
    m_pRenderPass = make<CRendererTest>();
    m_pRenderPass->init(AppInfo, ERenderPassPos::BEGIN);
    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderPass->getCamera());

    m_pGUI->setCamera(m_pRenderPass->getCamera());
    m_pGUI->setRenderer(m_pRenderPass);
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pGUI->update(vImageIndex);
    m_pRenderPass->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> BufferSet;
    std::vector<VkCommandBuffer> RendererBufferSet = m_pRenderPass->requestCommandBuffers(vImageIndex);
    BufferSet.insert(BufferSet.end(), RendererBufferSet.begin(), RendererBufferSet.end());
    std::vector<VkCommandBuffer> GUIBufferSet = m_pGUI->requestCommandBuffers(vImageIndex);
    BufferSet.insert(BufferSet.end(), GUIBufferSet.begin(), GUIBufferSet.end());
    return BufferSet;
}

void CApplicationTest::_createOtherResourceV()
{
    m_pGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
    m_pRenderPass->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
}

void CApplicationTest::_recreateOtherResourceV()
{
    m_pGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
    m_pRenderPass->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
}

void CApplicationTest::_destroyOtherResourceV()
{
    m_pGUI->destroy();
    m_pRenderPass->destroy();
}