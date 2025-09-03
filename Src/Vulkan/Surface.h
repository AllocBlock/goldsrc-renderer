#pragma once
#include "PchVulkan.h"
#include "Instance.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace vk
{
    class CSurface : public IVulkanHandle<VkSurfaceKHR>
    {
    public:
        
        void create(cptr<CInstance> vInstance, GLFWwindow* vWindow);
        void destroy();

        GLFWwindow* getWindow() const { return m_pWindow; }

    private:
        cptr<CInstance> m_pInstance = nullptr;
        GLFWwindow* m_pWindow = nullptr;
    };
}

