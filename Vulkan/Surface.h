#pragma once
#include "VulkanHandle.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vk
{
    class CSurface : public IVulkanHandle<VkSurfaceKHR>
    {
    public:
        _DEFINE_PTR(CSurface);

        void create(VkInstance vInstance, GLFWwindow* vWindow);
        void destroy();
    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
    };
}

