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
        _DEFINE_PTR(CSurface);

        void create(CInstance::CPtr vInstance, GLFWwindow* vWindow);
        void destroy();

        GLFWwindow* getWindow() const { return m_pWindow; }

    private:
        CInstance::CPtr m_pInstance = nullptr;
        GLFWwindow* m_pWindow = nullptr;
    };
}

