#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <set>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using std::string;
using std::array;
using std::vector;
using std::optional;
using std::set;
using glm::vec3;

const int MAX_FRAMES_IN_FLIGHT = 2;

struct SQueueFamilyIndices {
    optional<uint32_t> GraphicsFamily;
    optional<uint32_t> PresentFamily;

    bool isComplete() {
        return GraphicsFamily.has_value() && PresentFamily.has_value();
    }
};

struct SSwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR Capabilities = {};
    vector<VkSurfaceFormatKHR> Formats;
    vector<VkPresentModeKHR> PresentModes;
};

struct SVertex {
    vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription() {
        // ��������������ݰ�λ���Լ�ÿ��������Ŀ�Ĵ�С
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SVertex);
        // The stride parameter specifies the number of bytes from one entry to the next, and the inputRate parameter can have one of the following values:
        // VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
        // VK_VERTEX_INPUT_RATE_INSTANCE : Move to the next data entry after each m_Instance
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
        // ������ν��Ͷ�������
        // An attribute description�ṹ��������δ�һ��chunk������ȡ��Ӧ���� 
        // ÿ��attributes���Զ���Ҫ��������

        array<VkVertexInputAttributeDescription, 1> AttributeDescriptions = {};

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SVertex, Pos);

        return AttributeDescriptions;
    }
};

struct SUniformBufferObject {
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Proj;
};

class CVulkanApp {
public:
    const vector<SVertex> m_Vertices = {
        {{-0.5f, -0.5f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}},
    };

    const vector<uint16_t> m_Indices = {
        0, 1, 2, 2, 3, 0,
    };

    CVulkanApp();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

    void                    initVulkan(vector<const char*> vExtensions);
    void                    initWindow();
    void                    mainLoop();
    void                    cleanup();
    void                    drawFrame();
    void                    updateUniformBuffer(uint32_t vImageIndex);

    GLFWwindow* getWindowPointer() { return m_pWindow; }
    const VkSurfaceKHR getSurface() const { return m_Surface; }

public:
    const vector<const char*> m_ValidationLayers = {
        //"VK_LAYER_KHRONOS_validation"
        "VK_LAYER_LUNARG_standard_validation"
    };

    const vector<const char*> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    GLFWwindow* m_pWindow = nullptr;
    VkInstance                      m_Instance;
    vector<VkPhysicalDevice>        m_PhysicalDevices;
    VkDebugUtilsMessengerEXT        m_DebugMessenger;
    VkPhysicalDevice                m_PhysicalDevice;
    VkDevice                        m_Device;
    VkQueue                         m_GraphicsQueue;
    VkQueue                         m_PresentQueue;
    uint32_t                        m_GraphicsQueueIndex;
    uint32_t                        m_PresentQueueIndex;
    VkSurfaceKHR                    m_Surface;
    VkSwapchainKHR                  m_SwapChain;
    vector<VkImage>                 m_SwapChainImages;
    VkFormat                        m_SwapChainImageFormat;
    VkExtent2D                      m_SwapChainExtent;
    vector<VkImageView>             m_SwapChainImageViews;
    VkRenderPass                    m_RenderPass;
    VkDescriptorSetLayout           m_DescriptorSetLayout;
    VkDescriptorPool                m_DescriptorPool;
    vector<VkDescriptorSet>         m_DescriptorSets;
    VkPipelineLayout                m_PipelineLayout;
    VkPipeline                      m_GraphicsPipeline;
    vector<VkFramebuffer>           m_SwapChainFramebuffers;
    VkCommandPool                   m_CommandPool;
    vector<VkCommandBuffer>         m_CommandBuffers;
    vector<VkSemaphore>             m_ImageAvailableSemaphores;
    vector<VkSemaphore>             m_RenderFinishedSemaphores;
    vector<VkFence>                 m_InFlightFences;
    size_t                          m_CurrentFrame = 0;
    bool                            m_FramebufferResized = false;
    VkBuffer                        m_VertexBuffer;
    VkDeviceMemory                  m_VertexBufferMemory;
    VkBuffer                        m_IndexBuffer;
    VkDeviceMemory                  m_IndexBufferMemory;
    vector<VkBuffer>                m_UniformBuffers;
    vector<VkDeviceMemory>          m_UniformBuffersMemory;

    // ˽�к���
    VkResult                __createInstance(vector<const char*> vExtensions);
    VkResult                __choosePhysicalDevice();
    VkResult                __createDevice();
    VkResult                __createSurface();
    VkResult                __createSwapChain();
    VkResult                __createImageViews();
    VkResult                __createRenderPass();
    VkResult                __createDescriptorSetLayout();
    VkResult                __createGraphicsPipeline();
    VkResult                __createFramebuffers();
    void                    __createVertexBuffer();
    void                    __createIndexBuffer();
    void                    __createUniformBuffers();
    VkResult                __createDescriptorPool();
    VkResult                __createDescriptorSets();
    VkResult                __createCommandPool();
    VkResult                __createCommandBuffers();
    VkResult                __createSemaphores();
    void                    __recreateSwapChain();

    VkResult                __createDebugUtilsMessengerEXT(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT*, const VkAllocationCallbacks*, VkDebugUtilsMessengerEXT*);
    VkResult                    __setupDebugMessenger();
    void                    __destroyDebugUtilsMessengerEXT(VkInstance m_Instance, VkDebugUtilsMessengerEXT m_DebugMessenger, const VkAllocationCallbacks* pAllocator);
    bool                    __isDeviceSuitable(VkPhysicalDevice m_Device);

    // ���ߺ���
    bool                    __isDeviceExtensionSupport(VkPhysicalDevice);
    bool                    __isValidationLayerSupport();
    SQueueFamilyIndices        __findQueueFamilies(const VkPhysicalDevice& vDevice);
    SSwapChainSupportDetails __querySwapChainSupport(VkPhysicalDevice m_Device);
    VkSurfaceFormatKHR        __chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>&);
    VkPresentModeKHR        __chooseSwapPresentMode(const vector<VkPresentModeKHR>&);
    VkExtent2D                __chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);
    VkResult                __createShaderModule(VkShaderModule& voShaderModule, const vector<char>& vCode);
    void                    __cleanupSwapChain();
    static void                framebufferResizeCallback(GLFWwindow* m_Window, int width, int height);
    uint32_t                __findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkResult                    __createBuffer(VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory, VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties);
    VkCommandBuffer            __beginSingleTimeCommands();
    void                    __endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void                    __copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkResult                __createImageView(VkImageView& voImageView, VkImage vImage, VkFormat vFormat, VkImageAspectFlags vAspectFlags);
    VkFormat                __findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat                __findDepthFormat();
    void                    __getPhysicalDeviceInfo(VkPhysicalDeviceProperties&, VkPhysicalDeviceFeatures&, VkPhysicalDeviceMemoryProperties&);


    // ��̬����
    static vector<char>        readFile(const string&);
};