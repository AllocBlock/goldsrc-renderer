#pragma once
#include "Common.h"
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

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions(1);

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(SSkyPointData, Pos);

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

struct CPipeline
{
public:
    CPipeline() = delete;
    CPipeline(VkPrimitiveTopology vPrimitiveToplogy, std::filesystem::path vVertShaderPath, std::filesystem::path vFragShaderPath) : m_PrimitiveToplogy(vPrimitiveToplogy), m_VertShaderPath(vVertShaderPath), m_FragShaderPath(vFragShaderPath) {}

    void destory()
    {
        if (m_Device == VK_NULL_HANDLE) return;

        vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
        vkDestroyPipelineLayout(m_Device, m_Layout, nullptr);
        m_Device = VK_NULL_HANDLE;
        m_Pipeline = VK_NULL_HANDLE;
        m_Layout = VK_NULL_HANDLE;
    }

    void create(VkDevice vDevice, VkRenderPass vRenderPass, VkVertexInputBindingDescription vInputBindingDescription, std::vector<VkVertexInputAttributeDescription> vInputAttributeDescriptions, VkExtent2D vExtent, VkDescriptorSetLayout vDescriptorSetLayout, VkPipelineDepthStencilStateCreateInfo vDepthStencilInfo, VkPipelineColorBlendStateCreateInfo vBlendInfo, std::optional<VkPipelineDynamicStateCreateInfo> vDynamicStateInfo = std::nullopt, std::optional<VkPushConstantRange> vPushConstantState = std::nullopt)
    {
        m_Device = vDevice;

        auto VertShaderCode = Common::readFile(m_VertShaderPath);
        auto FragShaderCode = Common::readFile(m_FragShaderPath);

        VkShaderModule VertShaderModule = Common::createShaderModule(m_Device, VertShaderCode);
        VkShaderModule FragShaderModule = Common::createShaderModule(m_Device, FragShaderCode);

        VkPipelineShaderStageCreateInfo VertShaderStageInfo = {};
        VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertShaderStageInfo.module = VertShaderModule;
        VertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo FragShaderStageInfo = {};
        FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragShaderStageInfo.module = FragShaderModule;
        FragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo,FragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputInfo.vertexBindingDescriptionCount = 1;
        VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vInputAttributeDescriptions.size());
        VertexInputInfo.pVertexBindingDescriptions = &vInputBindingDescription;
        VertexInputInfo.pVertexAttributeDescriptions = vInputAttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
        InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyInfo.topology = m_PrimitiveToplogy;
        InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkViewport Viewport = {};
        Viewport.width = static_cast<float>(vExtent.width);
        Viewport.height = static_cast<float>(vExtent.height);
        Viewport.minDepth = 0.0f;
        Viewport.maxDepth = 1.0f;

        VkRect2D Scissor = {};
        Scissor.offset = { 0, 0 };
        Scissor.extent = vExtent;

        VkPipelineViewportStateCreateInfo ViewportStateInfo = {};
        ViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportStateInfo.viewportCount = 1;
        ViewportStateInfo.pViewports = &Viewport;
        ViewportStateInfo.scissorCount = 1;
        ViewportStateInfo.pScissors = &Scissor;

        VkPipelineRasterizationStateCreateInfo RasterizerInfo = {};
        RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizerInfo.depthClampEnable = VK_FALSE;
        RasterizerInfo.depthBiasEnable = VK_TRUE;
        RasterizerInfo.depthBiasConstantFactor = 0.0;
        RasterizerInfo.depthBiasSlopeFactor = 1.0;
        RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
        RasterizerInfo.lineWidth = 1.0f;
        RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo Multisampling = {};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
        PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount = 1;
        PipelineLayoutInfo.pSetLayouts = &vDescriptorSetLayout;
        if (vPushConstantState.has_value())
        {
            PipelineLayoutInfo.pushConstantRangeCount = 1;
            PipelineLayoutInfo.pPushConstantRanges = &vPushConstantState.value();
        }
        else
        {
            PipelineLayoutInfo.pushConstantRangeCount = 0;
            PipelineLayoutInfo.pPushConstantRanges = nullptr;
        }

        ck(vkCreatePipelineLayout(m_Device, &PipelineLayoutInfo, nullptr, &m_Layout));

        VkGraphicsPipelineCreateInfo PipelineInfo = {};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineInfo.stageCount = 2;
        PipelineInfo.pStages = ShaderStages;
        PipelineInfo.pVertexInputState = &VertexInputInfo;
        PipelineInfo.pInputAssemblyState = &InputAssemblyInfo;
        PipelineInfo.pViewportState = &ViewportStateInfo;
        PipelineInfo.pRasterizationState = &RasterizerInfo;
        PipelineInfo.pMultisampleState = &Multisampling;
        PipelineInfo.pDepthStencilState = &vDepthStencilInfo;
        PipelineInfo.pColorBlendState = &vBlendInfo;
        if (vDynamicStateInfo.has_value())
            PipelineInfo.pDynamicState = &vDynamicStateInfo.value();
        else
            PipelineInfo.pDynamicState = nullptr;
        PipelineInfo.layout = m_Layout;
        PipelineInfo.renderPass = vRenderPass;
        PipelineInfo.subpass = 0;

        ck(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr,&m_Pipeline));

        vkDestroyShaderModule(m_Device, FragShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, VertShaderModule, nullptr);
    }

    void bind(VkCommandBuffer vCommandBuffer, VkDescriptorSet vDescSet)
    {
        vkCmdBindPipeline(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        vkCmdBindDescriptorSets(vCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0, 1, &vDescSet, 0, nullptr);
    }

    template <typename T>
    void pushConstant(VkCommandBuffer vCommandBuffer, VkShaderStageFlags vState, T vPushConstant)
    {
        vkCmdPushConstants(vCommandBuffer, m_Layout, vState, 0, sizeof(vPushConstant), &vPushConstant);
    }

private:
    VkPrimitiveTopology m_PrimitiveToplogy = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    std::filesystem::path m_VertShaderPath;
    std::filesystem::path m_FragShaderPath;

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
};

struct SPipelineSet
{
    CPipeline TrianglesWithDepthTest;
    CPipeline TrianglesWithBlend;
    CPipeline TrianglesSky;

    void destory()
    {
        TrianglesWithDepthTest.destory();
        TrianglesWithBlend.destory();
        TrianglesSky.destory();
    }
};

struct SSkyBox
{
    bool IsInited = false;
    Common::SVkImagePack SkyBoxImagePack; // cube
    Common::SVkBufferPack VertexData;
    size_t VertexNum = 0;
    std::vector<Common::SVkBufferPack> VertUniformBufferPacks;
    std::vector<Common::SVkBufferPack> FragUniformBufferPacks;
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
    void __updateSkyUniformBuffer(uint32_t vImageIndex);
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
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "shader/vert.spv", "shader/frag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "shader/vert.spv", "shader/frag.spv"},
        {VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "shader/skyVert.spv", "shader/skyFrag.spv"},
    };
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    std::vector<VkDescriptorSet> m_SkyDescriptorSets;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkSampler m_TextureSampler = VK_NULL_HANDLE;

    Common::SVkBufferPack m_VertexBufferPack;
    Common::SVkBufferPack m_IndexBufferPack;
    std::vector<Common::SVkBufferPack> m_VertUniformBufferPacks;
    std::vector<Common::SVkBufferPack> m_FragUniformBufferPacks;
    
    std::vector<Common::SVkImagePack> m_TextureImagePacks;
    Common::SVkImagePack m_DepthImagePack;
    Common::SVkImagePack m_LightmapImagePack;

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