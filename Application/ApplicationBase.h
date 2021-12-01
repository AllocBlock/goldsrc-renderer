#pragma once
#include "Common.h"
#include "Vulkan.h"
#include "Instance.h"
#include "DebugMessenger.h"
#include "Surface.h"
#include "Device.h"
#include "SwapChain.h"
#include "Fence.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vector>

#ifdef _DEBUG
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

class CApplicationBase
{
public:
    CApplicationBase() = default;

    void init(GLFWwindow* vWindow);
    void render();
    void waitDevice();
    void destroy();
    Vulkan::SVulkanAppInfo getAppInfo();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

protected:
    virtual void _initV();
    virtual void _updateV(uint32_t vImageIndex);
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex);
    virtual void _createOtherResourceV();
    virtual void _recreateOtherResourceV();
    virtual void _destroyOtherResourceV();

    GLFWwindow* m_pWindow = nullptr;
    std::shared_ptr<vk::CInstance> m_pInstance = std::make_shared<vk::CInstance>();;
    std::shared_ptr<vk::CDebugMessenger> m_pDebugMessenger = std::make_shared<vk::CDebugMessenger>();;
    std::shared_ptr<vk::CSurface> m_pSurface = std::make_shared<vk::CSurface>();;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    std::shared_ptr<vk::CDevice> m_pDevice = std::make_shared<vk::CDevice>();
    std::shared_ptr<vk::CSwapchain> m_pSwapchain = std::make_shared<vk::CSwapchain>();;

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<std::shared_ptr<vk::CFence>> m_InFlightFenceSet;

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
    void __choosePhysicalDevice();
    void __createSemaphores();
    void __createSwapchain();
    void __destroySwapchain();

    void __recreateSwapchain();

    std::vector<const char*> __getRequiredExtensions();
};

