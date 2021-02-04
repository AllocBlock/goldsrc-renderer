#pragma once
#include "Scene.h"
#include "Camera.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <optional>

enum class ERenderMethod
{
    BSP,
    DEPTH_TEST
};

struct SPointData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec2 LightmapCoord;
    uint32_t TexIndex;
    uint32_t LightmapIndex;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 7> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 7> AttributeDescriptions = {};

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

        AttributeDescriptions[4].binding = 0;
        AttributeDescriptions[4].location = 4;
        AttributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[4].offset = offsetof(SPointData, LightmapCoord);

        AttributeDescriptions[5].binding = 0;
        AttributeDescriptions[5].location = 5;
        AttributeDescriptions[5].format = VK_FORMAT_R32_UINT;
        AttributeDescriptions[5].offset = offsetof(SPointData, TexIndex);

        AttributeDescriptions[6].binding = 0;
        AttributeDescriptions[6].location = 6;
        AttributeDescriptions[6].format = VK_FORMAT_R32_UINT;
        AttributeDescriptions[6].offset = offsetof(SPointData, LightmapIndex);

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
    unsigned int NotUsedForNow;
};

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

class CVulkanRenderer
{
public:
    CVulkanRenderer();
    
    void init(VkInstance vInstance, VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, uint32_t vGraphicsFamilyIndex, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void update(uint32_t vImageIndex);
    void destroy();
    void loadScene(const SScene& vScene);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void rerecordCommand();
    std::shared_ptr<CCamera> getCamera();
    size_t getRenderedObjectNum() const { return m_VisableObjectNum; }

    bool getCullingState() const { return m_EnableCulling; }
    void setCullingState(bool vCullingState) { m_EnableCulling = vCullingState; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    void setFrustumCullingState(bool vFrustumCullingState) { m_EnableFrustumCulling = vFrustumCullingState; }
    bool getPVSState() const { return m_EnablePVS; }
    void setPVSState(bool vPVS) { m_EnablePVS = vPVS; }
    std::optional<uint32_t> getCameraNodeIndex() const { return m_CameraNodeIndex; }
    std::vector<uint32_t> getRenderNodeList() const { return m_RenderNodeList; }

private:
    void __createRenderPass();
    void __createDescriptorSetLayout();
    void __createGraphicsPipeline();
    void __createCommandPool();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createTextureImageViews();
    void __createLightmapImages();
    void __createLightmapImageViews();
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

    void __renderByBspTree(uint32_t vImageIndex);
    void __renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex);
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const;
    
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
    size_t __getActualLightmapNum();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, VkImage& voImage, VkDeviceMemory& voImageMemory);
    void __updateDescriptorSets();

    std::vector<char> __readFile(std::filesystem::path vFilePath);
    std::vector<SPointData> __readPointData(std::shared_ptr<S3DObject> vpObject) const;

    VkInstance m_Instance = VK_NULL_HANDLE;
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
    std::vector<VkImage> m_LightmapImages;
    std::vector<VkDeviceMemory> m_LightmapImageMemories;
    std::vector<VkImageView> m_LightmapImageViews;
    VkSampler m_TextureSampler = VK_NULL_HANDLE;

    VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = { 0, 0 };
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    std::string m_VertShaderPath = "shader/vert.spv";
    std::string m_FragShaderPath = "shader/frag.spv";
    VkPrimitiveTopology m_DefaultPrimitiveToplogy = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    SScene m_Scene;
    size_t m_RerecordCommand = 0;
    std::vector<bool> m_AreObjectsVisable;
    size_t m_VisableObjectNum;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    std::shared_ptr<CCamera> m_pCamera = nullptr;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::vector<uint32_t> m_RenderNodeList;
    ERenderMethod m_RenderMethod = ERenderMethod::BSP;

    const float m_WindowWidth = 800;
    const float m_WindowHeight = 600;
    const size_t m_MaxTextureNum = 2048; // if need change, you should change this in frag shader as well
    const size_t m_MaxLightmapNum = 0x20000; // limitation from goldsrc. If need change, you should change this in frag shader as well

    bool m_FramebufferResized = false;
};