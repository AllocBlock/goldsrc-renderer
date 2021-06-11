#pragma once
#include "Command.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <memory>
#include <queue>
#include <filesystem>
#include <future>

#ifdef _DEBUG
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

class CGUIBase
{
public:
    CGUIBase() = delete;
    CGUIBase(GLFWwindow* vpWindow);

    void init(bool vHasRenderer);
    void render();
    std::vector<VkCommandBuffer> requestCommandBufferSet(uint32_t vImageIndex);
    void waitDevice();
    void destroy();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

protected:
    virtual void _initV();
    virtual void _renderV(uint32_t vImageIndex);
    virtual std::vector<VkCommandBuffer> _getCommandBufferSetV(uint32_t vImageIndex);
    virtual void _createOtherResourceV();
    virtual void _destroyOtherResourceV();

    GLFWwindow* m_pWindow = nullptr;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Main";
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    VkFormat m_SwapchainImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkExtent2D m_SwapchainExtent = { 0, 0 };

    uint32_t m_GraphicsQueueIndex = 0;
    uint32_t m_PresentQueueFamily = 0;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFramebuffer> m_FrameBuffers;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

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
    void __destroyDebugMessenger();
    void __createSurface();
    void __choosePhysicalDevice();
    void __createDevice();
    void __createSwapchain();
    void __createSwapchainImageViews();
    void __createSemaphores();
    void __createDescriptorPool();
    void __createFramebuffers();
    void __drawGUI();

    void __recreateSwapchain();
    void __destroySwapchainResources();

    std::vector<const char*> __getRequiredExtensions();
    VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats);
    VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes);
    VkExtent2D __chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities);
};