#include "GUIMain.h"
#include "Common.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <set>

CGUIMain::CGUIMain(GLFWwindow* vpWindow) :
    CGUIBase(vpWindow)
{
    m_pRenderer = std::make_shared<CVulkanRenderer>();
    m_pInteractor = std::make_shared<CInteractor>(vpWindow, m_pRenderer);
    m_pInteractor->bindEvent();

    Common::Log::setLogObserverFunc([=](std::string vText)
    {
        m_GUILog.log(vText);
    });
}

void CGUIMain::showAlert(std::string vText)
{
    m_GUIAlert.appendAlert(vText);
    Common::Log::log(u8"警告: " + vText);
}

void CGUIMain::log(std::string vText)
{
    m_GUILog.log(vText);
}

void CGUIMain::_initV()
{
    m_pRenderer->init(m_Instance, m_PhysicalDevice, m_Device, m_GraphicsQueueIndex, m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViews);
}

void CGUIMain::_renderV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    __drawGUI();
    m_pRenderer->update(vImageIndex);
}

std::vector<VkCommandBuffer> CGUIMain::_getCommandBufferSetV(uint32_t vImageIndex)
{
    return { m_pRenderer->requestCommandBuffer(vImageIndex) };
}

void CGUIMain::_createOtherResourceV()
{
    m_pRenderer->recreate(m_SwapchainImageFormat, m_SwapchainExtent, m_SwapchainImageViews);
}

void CGUIMain::_destroyOtherResourceV()
{
    m_pRenderer->destroy();
}