#include "PchVulkan.h"
#include "Surface.h"
#include "Vulkan.h"

using namespace vk;

void CSurface::create(CInstance::CPtr vInstance, GLFWwindow* vWindow)
{
    destroy();

    m_pInstance = vInstance;
    m_pWindow = vWindow;
    vk::checkError(glfwCreateWindowSurface(*vInstance, vWindow, nullptr, _getPtr()));
}

void CSurface::destroy()
{
    if (get()) vkDestroySurfaceKHR(*m_pInstance, get(), nullptr);
    _setNull();
    m_pInstance = nullptr;
    m_pWindow = nullptr;
}