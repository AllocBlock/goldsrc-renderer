#pragma once
#include "Scene.h"
#include "Camera.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <optional>

enum class ERenderMethod
{
    DEFAULT,
    BSP
};

struct SVkImagePack
{
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;

    bool isValid()
    {
        return Image != VK_NULL_HANDLE && Memory != VK_NULL_HANDLE && ImageView != VK_NULL_HANDLE;
    }

    void destory(VkDevice vDevice)
    {
        vkDestroyImage(vDevice, Image, nullptr);
        vkFreeMemory(vDevice, Memory, nullptr);
        vkDestroyImageView(vDevice, ImageView, nullptr);
        Image = VK_NULL_HANDLE;
        Memory = VK_NULL_HANDLE;
        ImageView = VK_NULL_HANDLE;
    }
};

struct SVkBufferPack
{
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;

    bool isValid()
    {
        return Buffer != VK_NULL_HANDLE && Memory != VK_NULL_HANDLE;
    }

    void destory(VkDevice vDevice)
    {
        vkDestroyBuffer(vDevice, Buffer, nullptr);
        vkFreeMemory(vDevice, Memory, nullptr);
        Buffer = VK_NULL_HANDLE;
        Memory = VK_NULL_HANDLE;
    }
};

struct SPointData
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec2 LightmapCoord;
    uint32_t TexIndex;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 6> AttributeDescriptions = {};

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

        return AttributeDescriptions;
    }
};

struct SSkyPointData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SSkyPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 1> AttributeDescriptions = {};

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SSkyPointData, Pos);

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

struct SSkyUniformBufferObjectFrag
{
    alignas(16) glm::vec3 EyeDirection;
};

struct SPushConstant
{
    VkBool32 UseLightmap = VK_FALSE;
    float Opacity = 1.0;
};

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

struct SPipeline
{
    VkPrimitiveTopology PrimitiveToplogy = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipeline Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout Layout = VK_NULL_HANDLE;
    std::filesystem::path VertShaderPath;
    std::filesystem::path FragShaderPath;

    void destory(VkDevice vDevice)
    {
        vkDestroyPipeline(vDevice, Pipeline, nullptr);
        vkDestroyPipelineLayout(vDevice, Layout, nullptr);
        Pipeline = VK_NULL_HANDLE;
        Layout = VK_NULL_HANDLE;
    }
};

struct SPipelineSet
{
    SPipeline TrianglesWithDepthTest;
    SPipeline TrianglesWithBlend;
    SPipeline TrianglesSky;

    void destory(VkDevice vDevice)
    {
        TrianglesWithDepthTest.destory(vDevice);
        TrianglesWithBlend.destory(vDevice);
        TrianglesSky.destory(vDevice);
    }
};

struct SSkyBox
{
    bool IsInited = false;
    std::array<SVkImagePack, 6> SkyBoxImages; // Left, Right, Front, Back, Up, Down
    SVkBufferPack Vertices;
    VkDeviceSize BufferSize;
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

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vSkyState) { m_EnableSky = vSkyState && m_Scene.UseSkyBox; }
    bool getCullingState() const { return m_EnableCulling; }
    void setCullingState(bool vCullingState) { m_EnableCulling = vCullingState; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    void setFrustumCullingState(bool vFrustumCullingState) { m_EnableFrustumCulling = vFrustumCullingState; }
    bool getPVSState() const { return m_EnablePVS; }
    void setPVSState(bool vPVS) { m_EnablePVS = vPVS; }
    std::optional<uint32_t> getCameraNodeIndex() const { return m_CameraNodeIndex; }
    std::vector<uint32_t> getRenderNodeList() const { return m_RenderNodeList; }
    ERenderMethod getRenderMethod() const { return m_RenderMethod; }
    void setRenderMethod(ERenderMethod vRenderMethod) { m_RenderMethod = vRenderMethod; }

private:
    void __createRenderPass();
    void __createDescriptorSetLayout();
    void __createSkyDescriptorSetLayout();
    void __createGraphicsPipelines();
    void __createCommandPool();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createTextureImageViews();
    void __createLightmapImage();
    void __createLightmapImageView();
    void __createTextureSampler();
    void __createVertexBuffer();
    void __createIndexBuffer();
    void __createUniformBuffers();
    void __createDescriptorPool();
    void __createDescriptorSets();
    void __createSkyDescriptorSets();
    void __createCommandBuffers();

    void __createSkyPipeline();
    void __createDepthTestPipeline();
    void __createBlendPipeline();
    
    void __createRecreateResources();
    void __destroyRecreateResources();
    void __createSceneResources();
    void __destroySceneResources();
    void __createSkyBoxResources();
    void __destroySkyBoxResources();

    void __renderByBspTree(uint32_t vImageIndex);
    void __renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex);
    void __renderModels(uint32_t vImageIndex);
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const;
    std::pair< std::vector<size_t>, std::vector<size_t>> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    
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
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, VkImage& voImage, VkDeviceMemory& voImageMemory);
    void __updateDescriptorSets();
    void __updateSkyDescriptorSets();

    std::vector<char> __readFile(std::filesystem::path vFilePath);
    std::vector<SPointData> __readPointData(std::shared_ptr<S3DObject> vpObject) const;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueIndex = 0;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_SkyDescriptorSetLayout = VK_NULL_HANDLE;
    SPipelineSet m_PipelineSet = 
    {
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_NULL_HANDLE, VK_NULL_HANDLE, "shader/vert.spv", "shader/frag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_NULL_HANDLE, VK_NULL_HANDLE, "shader/vert.spv", "shader/frag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_NULL_HANDLE, VK_NULL_HANDLE, "shader/skyVert.spv", "shader/skyFrag.spv"},
    };
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<VkDescriptorSet> m_SkyDescriptorSets;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkSampler m_TextureSampler = VK_NULL_HANDLE;

    SVkBufferPack m_VertexBufferPack;
    SVkBufferPack m_IndexBufferPack;
    std::vector<SVkBufferPack> m_VertUniformBufferPacks;
    std::vector<SVkBufferPack> m_FragUniformBufferPacks;
    
    std::vector<SVkImagePack> m_TextureImagePacks;
    SVkImagePack m_DepthImagePack;
    SVkImagePack m_LightmapImagePack;

    VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = { 0, 0 };
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    SScene m_Scene;
    SSkyBox m_SkyBox;
    size_t m_RerecordCommand = 0;
    std::vector<bool> m_AreObjectsVisable;
    size_t m_VisableObjectNum;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    std::shared_ptr<CCamera> m_pCamera = nullptr;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::vector<uint32_t> m_RenderNodeList;
    ERenderMethod m_RenderMethod = ERenderMethod::BSP;

    const float m_WindowWidth = 800;
    const float m_WindowHeight = 600;
    const size_t m_MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

    bool m_FramebufferResized = false;
};