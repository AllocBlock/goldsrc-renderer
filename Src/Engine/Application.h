#pragma once
#include "Instance.h"
#include "DebugMessenger.h"
#include "Surface.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "SwapChain.h"
#include "Fence.h"
#include "RenderPassPort.h"
#include "RenderPassGraphInstance.h"

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
    IApplication();
    virtual ~IApplication() = default;

    void create(GLFWwindow* vWindow);
    void tick();
    void resize(uint32_t vWidth, uint32_t vHeight);
    void waitDevice();
    void destroy();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

protected:
    virtual void _createV() = 0;
    virtual void _updateV(uint32_t vImageIndex) = 0;
    virtual void _renderUIV() {}
    virtual std::vector<VkCommandBuffer> _getCommandBuffers() = 0;
    virtual void _destroyV() = 0;

    virtual void _onSwapchainRecreateV() {}

    GLFWwindow* m_pWindow = nullptr;
    sptr<vk::CInstance> m_pInstance = make<vk::CInstance>();
    sptr<vk::CDebugMessenger> m_pDebugMessenger = make<vk::CDebugMessenger>();
    sptr<vk::CSurface> m_pSurface = make<vk::CSurface>();
    sptr<vk::CPhysicalDevice> m_pPhysicalDevice = nullptr;
    sptr<vk::CDevice> m_pDevice = make<vk::CDevice>();
    sptr<vk::CSwapchain> m_pSwapchain = make<vk::CSwapchain>();

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    sptr<vk::CFence> m_pInFlightFence;

    int m_CurrentFrameIndex = 0;

    const std::vector<const char*> m_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME
    };
    
    const sptr<SSceneInfo> m_pSceneInfo = make<SSceneInfo>();

private:
    void __render();
    void __createInstance();
    void __setupDebugMessenger();
    void __createSemaphores();

    void __recreateSwapchain();
    std::vector<VkCommandBuffer> __sortCommandBuffers(uint32_t vImageIndex);

    std::vector<const char*> __getRequiredExtensions();

    bool m_Freezed = false; // when freezed, no rendering is performed
};
