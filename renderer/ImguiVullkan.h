#pragma once

#include "Interactor.h"
#include "VulkanRenderer.h"
#include "GUIAlert.h"
#include "GUIFrameRate.h"

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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

struct SResultReadScene
{
    bool Succeed = false;;
    SScene Scene;
    std::string Message;
};

class CImguiVullkan
{
public:
    CImguiVullkan() = delete;
    CImguiVullkan(GLFWwindow* vpWindow);

    void init();
    void render();
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void waitDevice();
    void destroy();
    void showAlert(std::string vText);

    static SResultReadScene readScene(std::filesystem::path vFilePath, std::function<void(std::string)> vProgressReportFunc = nullptr);

    std::shared_ptr<CVulkanRenderer> getRenderer() { return m_pRenderer; }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

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

    GLFWwindow* m_pWindow = nullptr;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
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
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkFramebuffer> m_FrameBuffers;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    const int m_MaxFrameInFlight = 2;
    int m_CurrentFrameIndex = 0;
    bool m_FramebufferResized = false;

    std::shared_ptr<CVulkanRenderer> m_pRenderer = nullptr;
    std::shared_ptr<CInteractor> m_pInteractor = nullptr;

    CGUIAlert m_GUIAlert = CGUIAlert();
    CGUIFrameRate m_GUIFrameRate = CGUIFrameRate();

    std::filesystem::path m_LoadingFilePath = "";
    std::string m_LoadingProgressReport = "";
    std::future<SResultReadScene> m_FileReadingPromise;

    const std::vector<const char*> m_ValidationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_DeviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, // required by extented dynamic state
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME // for depth test enable cmd
    };
};