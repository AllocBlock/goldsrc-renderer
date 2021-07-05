#include "ApplicationGoldSrc.h"
#include "Common.h"

#include <iostream>
#include <set>

void CApplicationGoldSrc::_initV()
{
}

void CApplicationGoldSrc::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pGUI->update(vImageIndex);
    m_pRenderer->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationGoldSrc::_getCommandBufferSetV(uint32_t vImageIndex)
{
    return
    { 
        m_pRenderer->requestCommandBuffer(vImageIndex),
        m_pGUI->requestCommandBuffer(vImageIndex)
    };
}

void CApplicationGoldSrc::_createOtherResourceV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    m_pGUI = std::make_shared<CGUIMain>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, ERendererPos::END);
    m_pRenderer = std::make_shared<CVulkanRenderer>();
    m_pRenderer->init(AppInfo, ERendererPos::BEGIN);
    m_pInteractor = std::make_shared<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderer);
    m_pGUI->setRenderer(m_pRenderer);
    m_pGUI->setInteractor(m_pInteractor);

    _recreateOtherResourceV();
}

void CApplicationGoldSrc::_recreateOtherResourceV()
{
    m_pGUI->recreate(m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViewSet);
    m_pRenderer->recreate(m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViewSet);
}

void CApplicationGoldSrc::_destroyOtherResourceV()
{
    m_pGUI->destroy();
    m_pRenderer->destroy();
}