#pragma once
#include "AppInfo.h"
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
    virtual void _destroyV() = 0;

    virtual void _onSwapchainRecreateV() {}

    GLFWwindow* m_pWindow = nullptr;
    vk::CInstance::Ptr m_pInstance = make<vk::CInstance>();
    ptr<vk::CDebugMessenger> m_pDebugMessenger = make<vk::CDebugMessenger>();
    vk::CSurface::Ptr m_pSurface = make<vk::CSurface>();
    vk::CPhysicalDevice::Ptr m_pPhysicalDevice = nullptr;
    vk::CDevice::Ptr m_pDevice = make<vk::CDevice>();
    vk::CSwapchain::Ptr m_pSwapchain = make<vk::CSwapchain>();
    const CSourcePort::Ptr m_pSwapchainPort = make<CSourcePort>("SwapChain", SPortFormat::createAnyOfUsage(EUsage::UNDEFINED), nullptr);

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<vk::CFence::Ptr> m_InFlightFenceSet;

    const int m_MaxFrameInFlight = 2;
    int m_CurrentFrameIndex = 0;

    const std::vector<const char*> m_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    const CAppInfo::Ptr m_pAppInfo = make<CAppInfo>();
    const ptr<SSceneInfo> m_pSceneInfo = make<SSceneInfo>();
    const CRenderPassGraphInstance::Ptr m_pGraphInstance = make<CRenderPassGraphInstance>();

private:
    void __render();
    void __createInstance();
    void __setupDebugMessenger();
    void __createSemaphores();
    void __createSwapchain();
    void __destroySwapchain();

    void __recreateSwapchain();
    std::vector<VkCommandBuffer> __sortCommandBuffers(uint32_t vImageIndex);

    std::vector<const char*> __getRequiredExtensions();

    bool m_Freezed = false; // when freezed, no rendering is performed
};
