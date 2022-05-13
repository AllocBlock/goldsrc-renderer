#pragma once
#include "VulkanHandle.h"
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
    private:
        CInstance::CPtr m_Instance = nullptr;
    };
}

