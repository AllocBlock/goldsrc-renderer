#pragma once
#include "Common.h"
#include "Scene.h"
#include "Camera.h"
#include "PipelineSkybox.h"
#include "PipelineDepthTest.h"
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

struct SGuiUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
};

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

struct SPipelineSet
{
    CPipelineDepthTest TrianglesWithDepthTest;
    CPipelineBase TrianglesWithBlend;
    CPipelineSkybox TrianglesSky;
    CPipelineBase GuiLines;

    void destroy()
    {
        TrianglesWithDepthTest.destroy();
        TrianglesWithBlend.destroy();
        TrianglesSky.destroy();
        GuiLines.destroy();
    }
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
    Common::SBufferPack VertexDataPack;
    std::vector<Common::SBufferPack> VertUniformBufferPacks;

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
    void stageFillBuffer(const void* vData, VkDeviceSize vSize, Common::SBufferPack& voTargetBufferPack);
    void stageFillImage(const void* vData, VkDeviceSize vSize, VkImageCreateInfo vImageInfo, Common::SImagePack& voTargetImagePack);
    size_t __getActualTextureNum();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, Common::SImagePack& voImagePack);
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

    Common::SBufferPack m_VertexBufferPack;
    Common::SBufferPack m_IndexBufferPack;
    Common::SImagePack m_PlaceholderImagePack;
    std::vector<Common::SImagePack> m_TextureImagePacks;
    Common::SImagePack m_DepthImagePack;
    Common::SImagePack m_LightmapImagePack;

    std::shared_ptr<SScene> m_pScene;
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
};