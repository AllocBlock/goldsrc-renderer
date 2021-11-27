#include "ApplicationTest.h"

void CApplicationTest::_initV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    m_pGUI = std::make_shared<CGUITest>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, ERendererPos::END);
    m_pRenderer = std::make_shared<CRendererTest>();
    m_pRenderer->init(AppInfo, ERendererPos::BEGIN);
    m_pInteractor = std::make_shared<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderer);
    /*m_pGUI->setRenderer(m_pRenderer);
    m_pGUI->setInteractor(m_pInteractor);*/
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pGUI->update(vImageIndex);
    m_pRenderer->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pRenderer->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    m_pGUI->recreate(m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViewSet);
    m_pRenderer->recreate(m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViewSet);
}

void CApplicationTest::_destroyOtherResourceV()
{
    m_pGUI->destroy();
    m_pRenderer->destroy();
}
