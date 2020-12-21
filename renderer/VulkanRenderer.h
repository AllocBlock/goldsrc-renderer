#pragma once
#include "IOObj.h"
#include "Camera.h"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <array>
#include <optional>

#define DEBUG_ON true

#ifdef DEBUG_ON
const bool ENABLE_VALIDATION_LAYERS = true;
#else
const bool ENABLE_VALIDATION_LAYERS = false;
#endif

#define ck(E) do{if (E) throw "vulkan error happened";} while(0)

struct SQueueFamilyIndices
{
    std::optional<uint32_t> GraphicsFamilyIndex;
    std::optional<uint32_t> PresentFamilyIndex;

    bool isComplete() {
        return GraphicsFamilyIndex.has_value() && PresentFamilyIndex.has_value();
    }
};

struct SSwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;
};

struct SPointData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 Normal;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> AttributeDescriptions = {};

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SPointData, Pos);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(SPointData, Color);

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[2].offset = offsetof(SPointData, Normal);

        return AttributeDescriptions;
    }
};

struct SUniformBufferObject
{
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};

class CVulkanRenderer
{
public:
    CVulkanRenderer(GLFWwindow* vpWindow, CCamera* vpCamera);
    ~CVulkanRenderer();

    void init();
    void readData(std::string vFileName);
    void render();
    void waitDevice();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT vMessageSeverity, VkDebugUtilsMessageTypeFlagsEXT vMessageType, const VkDebugUtilsMessengerCallbackDataEXT* vpCallbackData, void* vpUserData);

private:
    void __createInstance();
    void __createSurface();
    void __choosePhysicalDevice();
    void __createDevice();
    void __createSwapChain();
    void __createImageViews();
    void __createRenderPass();
    void __createDescriptorSetLayout();
    void __createGraphicsPipeline();
    void __createCommandPool();
    void __createDepthResources();
    void __createFramebuffers();
    void __createVertexBuffer();
    void __createIndexBuffer();
    void __createUniformBuffers();
    void __createDescriptorPool();
    void __createDescriptorSets();
    void __createCommandBuffers();
    void __createSemaphores();

    void __cleanupSwapChain();
    void __recreateSwapChain();
    void __updateUniformBuffer(uint32_t vCurrentImage);
    
    bool __checkValidationLayerSupport();
    std::vector<const char*> __getRequiredExtensions();
    bool __isDeviceSuitable(const VkPhysicalDevice& vPhysicalDevice);
    SQueueFamilyIndices __findQueueFamilies(const VkPhysicalDevice& vPhysicalDevice);
    bool __checkDeviceExtensionSupport(const VkPhysicalDevice& vPhysicalDevice);
    SSwapChainSupportDetails __getSwapChainSupport(const VkPhysicalDevice& vPhysicalDevice);
    VkSurfaceFormatKHR __chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& vAvailableFormats);
    VkPresentModeKHR __chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& vAvailablePresentModes);
    VkExtent2D __chooseSwapExtent(const VkSurfaceCapabilitiesKHR& vCapabilities);
    VkImageView __createImageView(VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags);
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    void __createImage(uint32_t vWidth, uint32_t vHeight, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory);
    uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    VkCommandBuffer __beginSingleTimeCommands();
    void __endSingleTimeCommands(VkCommandBuffer vCommandBuffer);
    void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout);
    bool __hasStencilComponent(VkFormat vFormat);
    void __createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory);
    void __copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize);
    VkShaderModule __createShaderModule(const std::vector<char>& vShaderCode);

    void __setupDebugMessenger();
    void __destroyDebugMessenger();
    std::vector<char> __readFile(std::string vFileName);

    GLFWwindow* m_pWindow = nullptr;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    VkImage m_DepthImage = VK_NULL_HANDLE;
    VkImageView m_DepthImageView = VK_NULL_HANDLE;
    VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBufferMemories;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;

    std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_SwapchainExtent = { 0, 0 };
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_SwapchainFramebuffers;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    std::vector<float> m_VertexData;
    std::vector<uint32_t> m_IndexData;

    const std::vector<const char*> m_ValidationLayers = 
    {
        //"VK_LAYER_KHRONOS_validation"
        "VK_LAYER_LUNARG_standard_validation"
    };

    const std::vector<const char*> m_DeviceExtensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const float m_WindowWidth = 800;
    const float m_WindowHeight = 600;
    const int m_MaxFrameInFlight = 2;

    int m_CurrentFrameIndex = 0;
    bool m_FramebufferResized = false;

    CCamera* m_pCamera = nullptr;
};