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
}

std::vector<VkCommandBuffer> CApplicationGoldSrc::_getCommandBufferSetV(uint32_t vImageIndex)
{
    return m_pGUI->requestCommandBuffers(vImageIndex);
}

void CApplicationGoldSrc::_createOtherResourceV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    m_pInteractor = std::make_shared<CSceneInteractor>();
    m_pInteractor->bindEvent(m_pWindow);

    m_pGUI = std::make_shared<CGUIMain>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->setInteractor(m_pInteractor);
    m_pGUI->init(AppInfo, ERendererPos::END);

    _recreateOtherResourceV();
}

void CApplicationGoldSrc::_recreateOtherResourceV()
{
    m_pGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
}

void CApplicationGoldSrc::_destroyOtherResourceV()
{
    m_pGUI->destroy();
}