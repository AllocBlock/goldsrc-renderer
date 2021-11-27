#include "Surface.h"

#include "Vulkan.h"

using namespace vk;

void CSurface::create(VkInstance vInstance, GLFWwindow* vWindow)
{
    destroy();

    m_Instance = vInstance;
    Vulkan::checkError(glfwCreateWindowSurface(vInstance, vWindow, nullptr, &m_Handle));
}

void CSurface::destroy()
{
    if (m_Handle) vkDestroySurfaceKHR(m_Instance, m_Handle, nullptr);
    m_Handle = VK_NULL_HANDLE;
}