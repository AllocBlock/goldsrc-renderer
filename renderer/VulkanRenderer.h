#pragma once
#include "Scene.h"
#include "Camera.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <optional>

struct SPointData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> AttributeDescriptions = {};

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

        AttributeDescriptions[3].binding = 0;
        AttributeDescriptions[3].location = 3;
        AttributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[3].offset = offsetof(SPointData, TexCoord);

        return AttributeDescriptions;
    }
};

struct SUniformBufferObjectVert
{
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};

struct SUniformBufferObjectFrag
{
    alignas(16) glm::vec3 Eye;
};

struct SPushConstant
{
    alignas(16) unsigned int TexIndex;
};

class CVulkanRenderer
{
public:
    CVulkanRenderer();
    
    void init(VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, uint32_t vGraphicsFamilyIndex, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void update(uint32_t vImageIndex);
    void destroy();
    void loadScene(const SScene& vScene);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    std::shared_ptr<CCamera> getCamera();

private:
    void __createRenderPass();
    void __createDescriptorSetLayout();
    void __createGraphicsPipeline();
    void __createCommandPool();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createTextureImageViews();
    void __createTextureSampler();
    void __createVertexBuffer();
    void __createIndexBuffer();
    void __createUniformBuffers();
    void __createDescriptorPool();
    void __createDescriptorSets();
    void __createCommandBuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();
    void __createSceneResources();
    void __destroySceneResources();

    void __updateUniformBuffer(uint32_t vImageIndex);
    
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    void __createImage(uint32_t vWidth, uint32_t vHeight, VkFormat vFormat, VkImageTiling vTiling, VkImageUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory);
    uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout);
    bool __hasStencilComponent(VkFormat vFormat);
    void __createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory);
    void __copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize);
    VkShaderModule __createShaderModule(const std::vector<char>& vShaderCode);
    void __copyBufferToImage(VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight);
    size_t __getActualTextureNum();
    void __updateDescriptorSets();

    std::vector<char> __readFile(std::string vFileName);
    std::vector<SPointData> __readPointData(S3DObject vObject) const;

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueIndex = 0;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;

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
    std::vector<VkBuffer> m_VertUniformBuffers;
    std::vector<VkDeviceMemory> m_VertUniformBufferMemories;
    std::vector<VkBuffer> m_FragUniformBuffers;
    std::vector<VkDeviceMemory> m_FragUniformBufferMemories;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    std::vector<VkImage> m_TextureImages;
    std::vector<VkDeviceMemory> m_TextureImageMemories;
    std::vector<VkImageView> m_TextureImageViews;
    VkSampler m_TextureSampler = VK_NULL_HANDLE;

    VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = { 0, 0 };
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    std::string m_VertShaderPath = "shader/vert.spv";
    std::string m_FragShaderPath = "shader/frag.spv";

    SScene m_Scene;

    const float m_WindowWidth = 800;
    const float m_WindowHeight = 600;
    const size_t m_MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

    bool m_FramebufferResized = false;

    std::shared_ptr<CCamera> m_pCamera = nullptr;
};