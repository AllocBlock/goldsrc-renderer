#pragma once
#include "Common.h"
#include "Scene.h"
#include "Camera.h"
#include "Pipeline.h"
#include "Descriptor.h"
#include "Command.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <array>
#include <optional>

enum class ERenderMethod
{
    DEFAULT,
    BSP
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

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions(6);

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

struct SSimplePointData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SSimplePointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions(1);

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SSimplePointData, Pos);

        return AttributeDescriptions;
    }
};

struct SUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
};

struct SUniformBufferObjectFrag
{
    alignas(16) glm::vec3 Eye;
};

struct SSkyUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::vec3 EyePosition;
};

struct SSkyUniformBufferObjectFrag
{
    alignas(16) glm::mat4 UpCorrection;
};

struct SGuiUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
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

struct SPipelineSet
{
    CPipeline TrianglesWithDepthTest;
    CPipeline TrianglesWithBlend;
    CPipeline TrianglesSky;
    CPipeline GuiLines;

    void destory()
    {
        TrianglesWithDepthTest.destory();
        TrianglesWithBlend.destory();
        TrianglesSky.destory();
        GuiLines.destory();
    }
};

struct SSkyBox
{
    bool IsInited = false;
    Common::SVkImagePack SkyBoxImagePack; // cube
    Common::SVkBufferPack VertexDataPack;
    size_t VertexNum = 0;
    std::vector<Common::SVkBufferPack> VertUniformBufferPacks;
    std::vector<Common::SVkBufferPack> FragUniformBufferPacks;
};

enum class EGuiObjectType
{
    LINE
};

struct SGuiObject
{
    EGuiObjectType Type = EGuiObjectType::LINE;
    std::vector<glm::vec3> Data;
};

struct SGui
{
    std::map<std::string, std::shared_ptr<SGuiObject>> NameObjectMap;
    Common::SVkBufferPack VertexDataPack;
    std::vector<Common::SVkBufferPack> VertUniformBufferPacks;

    void addOrUpdateObject(std::string vName, std::shared_ptr<SGuiObject> vpObject)
    {
        NameObjectMap[vName] = std::move(vpObject);
    }

    void removeObject(std::string vName)
    {
        NameObjectMap.erase(vName);
    }

    bool isEmpty() { return NameObjectMap.empty(); }
};

class CVulkanRenderer
{
public:
    CVulkanRenderer();
    
    void init(VkInstance vInstance, VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, uint32_t vGraphicsFamilyIndex, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews);
    void update(uint32_t vImageIndex);
    void destroy();
    std::shared_ptr<SScene> getScene() const { return m_pScene; }
    void loadScene(std::shared_ptr<SScene> vpScene);
    VkCommandBuffer requestCommandBuffer(uint32_t vImageIndex);
    void rerecordCommand();
    std::shared_ptr<CCamera> getCamera();
    size_t getRenderedObjectNum() const { return m_VisableObjectNum; }

    void setHighlightBoundingBox(S3DBoundingBox vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd); 

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vSkyState) { m_EnableSky = vSkyState && m_pScene->UseSkyBox; }
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
    void __createRenderPass(bool vPresentLayout);
    void __createDefaultDescriptorSetLayout();
    void __createSkyDescriptorSetLayout();
    void __createLineDescriptorSetLayout();
    void __createGraphicsPipelines();
    void __createCommandPoolAndBuffers();
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
    void __createDefaultDescriptorSets();
    void __createSkyDescriptorSets();
    void __createLineDescriptorSets();
    void __createPlaceholderImage();

    void __createSkyPipeline();
    void __createDepthTestPipeline();
    void __createBlendPipeline();
    void __createGuiLinesPipeline();
    
    void __createRecreateResources();
    void __destroyRecreateResources();
    void __createSceneResources();
    void __destroySceneResources();
    void __createSkyBoxResources();
    void __destroySkyBoxResources();
    void __createGuiResources();
    void __destroyGuiResources();

    void __renderByBspTree(uint32_t vImageIndex);
    void __renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex);
    void __renderModels(uint32_t vImageIndex);
    void __renderModel(uint32_t vImageIndex, size_t vModelIndex);
    void __updateUniformBuffer(uint32_t vImageIndex);
    void __updateSkyUniformBuffer(uint32_t vImageIndex);
    void __updateGuiUniformBuffer(uint32_t vImageIndex);
    void __recordGuiCommandBuffers();
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const;
    std::pair< std::vector<size_t>, std::vector<size_t>> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    void __createImage(VkImageCreateInfo vImageInfo, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory);
    uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount = 1);
    bool __hasStencilComponent(VkFormat vFormat);
    void __createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory);
    void __copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize);
    void __copyBufferToImage(VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight, uint32_t vLayerCount = 1);
    size_t __getActualTextureNum();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, VkImage& voImage, VkDeviceMemory& voImageMemory);
    void __updateDescriptorSets();
    void __updateSkyDescriptorSets();
    void __updateLineDescriptorSets();

    std::vector<SPointData> __readPointData(std::shared_ptr<S3DObject> vpObject) const;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueIndex = 0;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    CDescriptor m_DefaultDescriptor = CDescriptor();
    CDescriptor m_SkyDescriptor = CDescriptor();
    CDescriptor m_LineDescriptor = CDescriptor();
    SPipelineSet m_PipelineSet = 
    {
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "../Renderer/shader/vert.spv", "../Renderer/shader/frag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "../Renderer/shader/vert.spv", "../Renderer/../Renderer/shader/frag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "../Renderer/shader/skyVert.spv", "../Renderer/shader/skyFrag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST, "../Renderer/shader/lineVert.spv", "../Renderer/shader/lineFrag.spv"},
    };
    CCommand m_Command = CCommand();
    std::string m_SceneCommandName = "Scene";
    std::string m_GuiCommandName = "Gui";
    VkFormat m_ImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = { 0, 0 };
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    Common::SVkBufferPack m_VertexBufferPack;
    Common::SVkBufferPack m_IndexBufferPack;
    std::vector<Common::SVkBufferPack> m_VertUniformBufferPacks;
    std::vector<Common::SVkBufferPack> m_FragUniformBufferPacks;
    Common::SVkImagePack m_PlaceholderImagePack;
    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    std::vector<Common::SVkImagePack> m_TextureImagePacks;
    Common::SVkImagePack m_DepthImagePack;
    Common::SVkImagePack m_LightmapImagePack;

    std::shared_ptr<SScene> m_pScene;
    SSkyBox m_SkyBox;
    SGui m_Gui;
    std::shared_ptr<CCamera> m_pCamera = nullptr;

    bool m_FramebufferResized = false;
    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    size_t m_VisableObjectNum = 0;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::vector<uint32_t> m_RenderNodeList;
    ERenderMethod m_RenderMethod = ERenderMethod::BSP;

    size_t m_NumSwapchainImage = 0;

    const size_t m_MaxTextureNum = 2048; // if need change, you should change this in frag shader as well
};