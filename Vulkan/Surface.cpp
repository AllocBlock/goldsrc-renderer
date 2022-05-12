#include "Surface.h"
#include "Vulkan.h"

using namespace vk;

void CSurface::create(CInstance::CPtr vInstance, GLFWwindow* vWindow)
{
    destroy();

    m_Instance = vInstance;
    vk::checkError(glfwCreateWindowSurface(*vInstance, vWindow, nullptr, _getPtr()));
}

void CSurface::destroy()
{
    if (get()) vkDestroySurfaceKHR(*m_Instance, get(), nullptr);
    _setNull();
}