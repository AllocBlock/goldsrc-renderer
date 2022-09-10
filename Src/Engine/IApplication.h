#pragma once
#include "AppInfo.h"
#include "Vulkan.h"
#include "Instance.h"
#include "DebugMessenger.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "SwapChain.h"
#include "Fence.h"
#include "RenderPassPort.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>

#ifdef _DEBUG
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

class IApplication
{
public:
    IApplication() = default;

    void init(GLFWwindow* vWindow);
    void render();
    void waitDevice();
    void destroy();
    vk::SAppInfo getAppInfo();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

protected:
    virtual void _initV();
    virtual void _updateV(uint32_t vImageIndex);
    virtual void _renderUIV() {};
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex);
    virtual void _createOtherResourceV();
    virtual void _recreateOtherResourceV();
    virtual void _destroyOtherResourceV();

    GLFWwindow* m_pWindow = nullptr;
    vk::CInstance::Ptr m_pInstance = make<vk::CInstance>();
    ptr<vk::CDebugMessenger> m_pDebugMessenger = make<vk::CDebugMessenger>();
    vk::CSurface::Ptr m_pSurface = make<vk::CSurface>();
    vk::CPhysicalDevice::Ptr m_pPhysicalDevice = nullptr;
    vk::CDevice::Ptr m_pDevice = make<vk::CDevice>();
    vk::CSwapchain::Ptr m_pSwapchain = make<vk::CSwapchain>();
    CSourcePort::Ptr m_pSwapchainPort = nullptr;

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<vk::CFence::Ptr> m_InFlightFenceSet;

    const int m_MaxFrameInFlight = 2;
    int m_CurrentFrameIndex = 0;
    bool m_FramebufferResized = false;

    const std::vector<const char*> m_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

private:
    void __createInstance();
    void __setupDebugMessenger();
    void __createSemaphores();
    void __createSwapchain();
    void __destroySwapchain();

    void __recreateSwapchain();

    std::vector<const char*> __getRequiredExtensions();
};

