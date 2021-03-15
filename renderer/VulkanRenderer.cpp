#include "VulkanRenderer.h"
#include "Common.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

CVulkanRenderer::CVulkanRenderer()
    : m_pCamera(std::make_shared<CCamera>())
{
}

void CVulkanRenderer::init(VkInstance vInstance, VkPhysicalDevice vPhysicalDevice, VkDevice vDevice, uint32_t vGraphicsFamilyIndex, VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    m_Instance = vInstance;
    m_PhysicalDevice = vPhysicalDevice;
    m_Device = vDevice;
    m_GraphicsQueueIndex = vGraphicsFamilyIndex;
    m_ImageFormat = vImageFormat;
    m_Extent = vExtent;
    m_ImageViews = vImageViews;
    m_NumSwapchainImage = m_ImageViews.size();

    vkGetDeviceQueue(m_Device, m_GraphicsQueueIndex, 0, &m_GraphicsQueue);
    __createRenderPass();
    __createDescriptorSetLayout();
    __createSkyDescriptorSetLayout();
    __createLineDescriptorSetLayout();
    __createCommandPool();
    __createCommandBuffers();
    __createGuiCommandBuffers();
    __createTextureSampler();
    __createPlaceholderImage();
    __createRecreateResources();

    __createGuiResources();
    __recordGuiCommandBuffers();
}

void CVulkanRenderer::__createRecreateResources()
{
    __createGraphicsPipelines(); // extent
    __createDepthResources(); // extent
    __createFramebuffers(); // imageview, extent
    __createUniformBuffers(); // imageview
    __createDescriptorPool(); // imageview
    __createDescriptorSets(); // imageview
    __createSkyDescriptorSets();
    __createSceneResources();
}

void CVulkanRenderer::__destroyRecreateResources()
{
    m_DepthImagePack.destory(m_Device);

    for (auto& Framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(m_Device, Framebuffer, nullptr);
    m_Framebuffers.clear();

    m_PipelineSet.destory();

    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        m_VertUniformBufferPacks[i].destory(m_Device);
        m_FragUniformBufferPacks[i].destory(m_Device);
    }
    m_VertUniformBufferPacks.clear();
    m_FragUniformBufferPacks.clear();

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    m_DescriptorPool = VK_NULL_HANDLE;

    __destroySceneResources();
}

void CVulkanRenderer::__createSceneResources()
{
    __createTextureImages(); // scene
    __createTextureImageViews(); // scene
    __createLightmapImage(); // scene
    __createLightmapImageView(); // scene
    __updateDescriptorSets();
    __createVertexBuffer(); // scene
    __createIndexBuffer(); // scene

    m_EnableSky = m_EnableSky && m_Scene.UseSkyBox;

    if (m_Scene.UseSkyBox)
    {
        __createSkyBoxResources();
        __updateSkyDescriptorSets();
    }
}

void CVulkanRenderer::__destroySceneResources()
{
    __destroySkyBoxResources();

    for (size_t i = 0; i < m_TextureImagePacks.size(); ++i)
    {
        m_TextureImagePacks[i].destory(m_Device);
    }
    m_TextureImagePacks.clear();

    m_LightmapImagePack.destory(m_Device);
    m_IndexBufferPack.destory(m_Device);
    m_VertexBufferPack.destory(m_Device);
}

void CVulkanRenderer::__createSkyBoxResources()
{
    _ASSERTE(m_Scene.UseSkyBox);
    m_SkyBox.IsInited = true;

    // format 6 image into one cubemap image
    int TexWidth = m_Scene.SkyBoxImages[0]->getImageWidth();
    int TexHeight = m_Scene.SkyBoxImages[0]->getImageHeight();
    size_t SingleFaceImageSize = static_cast<size_t>(4) * TexWidth * TexHeight;
    size_t TotalImageSize = SingleFaceImageSize * 6;
    uint8_t* pPixelData = new uint8_t[TotalImageSize];
    memset(pPixelData, 0, TotalImageSize);
    /*
     * a cubemap image in vulkan has 6 faces(layers), and in sequence they are
     * +x, -x, +y, -y, +z, -z
     * 
     * in vulkan:
     * +y
     * +z +x -z -x
     * -y
     * 
     * cubemap face to outside(fold +y and -y behind)
     * in GoldSrc:
     * up
     * right front left back
     * down
     * in sequence: front back up down right left
     */
    
    for (size_t i = 0; i < m_Scene.SkyBoxImages.size(); ++i)
    {
        _ASSERTE(TexWidth == m_Scene.SkyBoxImages[i]->getImageWidth() && TexHeight == m_Scene.SkyBoxImages[i]->getImageHeight());
        const void* pData = m_Scene.SkyBoxImages[i]->getData();
        memcpy_s(pPixelData + i * SingleFaceImageSize, SingleFaceImageSize, pData, SingleFaceImageSize);
    }

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(TotalImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pDevData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, TotalImageSize, 0, &pDevData));
    memcpy(pDevData, pPixelData, TotalImageSize);
    vkUnmapMemory(m_Device, StagingBufferMemory);
    delete[] pPixelData;

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = TexWidth;
    ImageInfo.extent.height = TexHeight;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 6;
    ImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageInfo.flags = VkImageCreateFlagBits::VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // important for cubemap

    __createImage(ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_SkyBox.SkyBoxImagePack.Image, m_SkyBox.SkyBoxImagePack.Memory);
    __transitionImageLayout(m_SkyBox.SkyBoxImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);
    __copyBufferToImage(StagingBuffer, m_SkyBox.SkyBoxImagePack.Image, TexWidth, TexHeight, 6);
    __transitionImageLayout(m_SkyBox.SkyBoxImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);

    m_SkyBox.SkyBoxImagePack.ImageView = Common::createImageView(m_Device, m_SkyBox.SkyBoxImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE, 6);

    // create plane
    const std::vector<glm::vec3> Vertices =
    {
        { 1.0,  1.0,  1.0}, // 0
        {-1.0,  1.0,  1.0}, // 1
        {-1.0,  1.0, -1.0}, // 2
        { 1.0,  1.0, -1.0}, // 3
        { 1.0, -1.0,  1.0}, // 4
        {-1.0, -1.0,  1.0}, // 5
        {-1.0, -1.0, -1.0}, // 6
        { 1.0, -1.0, -1.0}, // 7
    };

    //const std::vector<SSimplePointData> PointData =
    //{
    //    {Vertices[0]}, {Vertices[3]}, {Vertices[2]}, {Vertices[0]}, {Vertices[2]}, {Vertices[1]}, // +y
    //    {Vertices[4]}, {Vertices[7]}, {Vertices[6]}, {Vertices[4]}, {Vertices[6]}, {Vertices[5]}, // -y
    //    {Vertices[4]}, {Vertices[7]}, {Vertices[3]}, {Vertices[4]}, {Vertices[3]}, {Vertices[0]}, // +x
    //    {Vertices[1]}, {Vertices[2]}, {Vertices[6]}, {Vertices[1]}, {Vertices[6]}, {Vertices[5]}, // -x
    //    {Vertices[4]}, {Vertices[0]}, {Vertices[1]}, {Vertices[4]}, {Vertices[1]}, {Vertices[5]}, // +z
    //    {Vertices[3]}, {Vertices[7]}, {Vertices[6]}, {Vertices[3]}, {Vertices[6]}, {Vertices[2]}, // -z
    //};

    const std::vector<SSimplePointData> PointData =
    {
        {Vertices[4]}, {Vertices[0]}, {Vertices[1]}, {Vertices[4]}, {Vertices[1]}, {Vertices[5]}, // +z
        {Vertices[3]}, {Vertices[7]}, {Vertices[6]}, {Vertices[3]}, {Vertices[6]}, {Vertices[2]}, // -z
        {Vertices[0]}, {Vertices[3]}, {Vertices[2]}, {Vertices[0]}, {Vertices[2]}, {Vertices[1]}, // +y
        {Vertices[5]}, {Vertices[6]}, {Vertices[7]}, {Vertices[5]}, {Vertices[7]}, {Vertices[4]}, // -y
        {Vertices[4]}, {Vertices[7]}, {Vertices[3]}, {Vertices[4]}, {Vertices[3]}, {Vertices[0]}, // +x
        {Vertices[1]}, {Vertices[2]}, {Vertices[6]}, {Vertices[1]}, {Vertices[6]}, {Vertices[5]}, // -x
    };

    VkDeviceSize DataSize = sizeof(SSimplePointData) * PointData.size();
    m_SkyBox.VertexNum = PointData.size();

   /* VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;*/
    __createBuffer(DataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, DataSize, 0, &pData));
    memcpy(reinterpret_cast<char*>(pData), PointData.data(), DataSize);
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(DataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_SkyBox.VertexDataPack.Buffer, m_SkyBox.VertexDataPack.Memory);

    __copyBuffer(StagingBuffer, m_SkyBox.VertexDataPack.Buffer, DataSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SSkyUniformBufferObjectVert);
    VkDeviceSize FragBufferSize = sizeof(SSkyUniformBufferObjectFrag);
    m_SkyBox.VertUniformBufferPacks.resize(m_NumSwapchainImage);
    m_SkyBox.FragUniformBufferPacks.resize(m_NumSwapchainImage);

    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        __createBuffer(VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_SkyBox.VertUniformBufferPacks[i].Buffer, m_SkyBox.VertUniformBufferPacks[i].Memory);
        __createBuffer(FragBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_SkyBox.FragUniformBufferPacks[i].Buffer, m_SkyBox.FragUniformBufferPacks[i].Memory);
    }
}

void CVulkanRenderer::__destroySkyBoxResources()
{
    m_SkyBox.SkyBoxImagePack.destory(m_Device);
    m_SkyBox.VertexDataPack.destory(m_Device);
    for (size_t i = 0; i < m_SkyBox.VertUniformBufferPacks.size(); ++i)
    {
        m_SkyBox.VertUniformBufferPacks[i].destory(m_Device);
        m_SkyBox.FragUniformBufferPacks[i].destory(m_Device);
    }
    m_SkyBox.VertUniformBufferPacks.clear();
    m_SkyBox.FragUniformBufferPacks.clear();
}

void CVulkanRenderer::__createGuiResources()
{
    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SGuiUniformBufferObjectVert);
    m_Gui.VertUniformBufferPacks.resize(m_NumSwapchainImage);

    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        __createBuffer(VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Gui.VertUniformBufferPacks[i].Buffer, m_Gui.VertUniformBufferPacks[i].Memory);
    }

    __createLineDescriptorSets();
    __updateLineDescriptorSets();
}

void CVulkanRenderer::__destroyGuiResources()
{
    m_Gui.VertexDataPack.destory(m_Device);
    for (auto& Buffer : m_Gui.VertUniformBufferPacks)
        Buffer.destory(m_Device);
}

void CVulkanRenderer::destroy()
{
    __destroyRecreateResources();
    __destroyGuiResources();

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_SceneCommandBuffers.size()), m_SceneCommandBuffers.data());
    m_SceneCommandBuffers.clear();
    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_GuiCommandBuffers.size()), m_GuiCommandBuffers.data());
    m_GuiCommandBuffers.clear();

    m_PlaceholderImagePack.destory(m_Device);
    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_LineDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_SkyDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    m_TextureSampler = VK_NULL_HANDLE;
    m_DescriptorSetLayout = VK_NULL_HANDLE;
    m_RenderPass = VK_NULL_HANDLE;
    m_CommandPool = VK_NULL_HANDLE;
}

void CVulkanRenderer::loadScene(const SScene& vScene)
{
     m_Scene = vScene;
     m_ObjectDataPositions.resize(m_Scene.Objects.size());
     if (m_Scene.BspTree.Nodes.empty())
         m_RenderMethod = ERenderMethod::DEFAULT;

     size_t IndexOffset = 0;
     size_t VertexOffset = 0;
     for (size_t i = 0; i < m_Scene.Objects.size(); ++i)
     {
         std::shared_ptr<S3DObject> pObject = m_Scene.Objects[i];
         if (pObject->DataType == E3DObjectDataType::INDEXED_TRIAGNLE_LIST)
         { 
             m_ObjectDataPositions[i].Offset = IndexOffset;
             m_ObjectDataPositions[i].Size = pObject->Indices.size();
             IndexOffset += m_ObjectDataPositions[i].Size;
         }
         else if (pObject->DataType == E3DObjectDataType::TRIAGNLE_LIST)
         {
             m_ObjectDataPositions[i].Offset = VertexOffset;
             m_ObjectDataPositions[i].Size = pObject->Vertices.size();
             VertexOffset += m_ObjectDataPositions[i].Size;
         }
         else
             throw std::runtime_error(u8"物体类型错误");
     }

     m_AreObjectsVisable.clear();
     m_AreObjectsVisable.resize(m_Scene.Objects.size(), false);
     m_VisableObjectNum = 0;

     vkDeviceWaitIdle(m_Device);
     __destroySceneResources();
     __createSceneResources();
}

VkCommandBuffer CVulkanRenderer::requestCommandBuffer(uint32_t vImageIndex)
{
    _ASSERTE(vImageIndex >= 0 && vImageIndex < m_SceneCommandBuffers.size());
    
    bool RerecordCommand = false;
    if (m_RenderMethod == ERenderMethod::BSP || m_EnableCulling || m_RerecordCommand > 0)
    {
        RerecordCommand = true;
        if (m_RerecordCommand > 0) --m_RerecordCommand;
    }
    if (RerecordCommand)
    {
        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        ck(vkBeginCommandBuffer(m_SceneCommandBuffers[vImageIndex], &CommandBufferBeginInfo));

        std::array<VkClearValue, 2> ClearValues = {};
        ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass = m_RenderPass;
        RenderPassBeginInfo.framebuffer = m_Framebuffers[vImageIndex];
        RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        RenderPassBeginInfo.renderArea.extent = m_Extent;
        RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
        RenderPassBeginInfo.pClearValues = ClearValues.data();

        vkCmdBeginRenderPass(m_SceneCommandBuffers[vImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (m_EnableSky)
            __recordSkyRenderCommand(vImageIndex);

        VkDeviceSize Offsets[] = { 0 };
        if (m_VertexBufferPack.isValid())
            vkCmdBindVertexBuffers(m_SceneCommandBuffers[vImageIndex], 0, 1, &m_VertexBufferPack.Buffer, Offsets);
        if (m_IndexBufferPack.isValid())
            vkCmdBindIndexBuffer(m_SceneCommandBuffers[vImageIndex], m_IndexBufferPack.Buffer, 0, VK_INDEX_TYPE_UINT32);
        
        if (m_VertexBufferPack.isValid() || m_IndexBufferPack.isValid())
        {
            __calculateVisiableObjects();
            if (m_RenderMethod == ERenderMethod::BSP)
                __renderByBspTree(vImageIndex);
            else
            {
                m_PipelineSet.TrianglesWithDepthTest.bind(m_SceneCommandBuffers[vImageIndex], m_DescriptorSets[vImageIndex]);
                
                SPushConstant PushConstant;
                PushConstant.Opacity = 1.0f;
                
                for (size_t i = 0; i < m_Scene.Objects.size(); ++i)
                {
                    PushConstant.UseLightmap = m_Scene.Objects[i]->HasLightmap;
                    m_PipelineSet.TrianglesWithDepthTest.pushConstant<SPushConstant>(m_SceneCommandBuffers[vImageIndex], VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
                    if (m_AreObjectsVisable[i])
                        __recordObjectRenderCommand(vImageIndex, i);
                }
            }
        }

        // GUI 
        vkCmdNextSubpass(m_SceneCommandBuffers[vImageIndex], VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        vkCmdExecuteCommands(m_SceneCommandBuffers[vImageIndex], 1, &m_GuiCommandBuffers[vImageIndex]);

        vkCmdEndRenderPass(m_SceneCommandBuffers[vImageIndex]);
        ck(vkEndCommandBuffer(m_SceneCommandBuffers[vImageIndex]));
    }
    return m_SceneCommandBuffers[vImageIndex];
}

void CVulkanRenderer::setHighlightBoundingBox(S3DBoundingBox vBoundingBox)
{
    vkDeviceWaitIdle(m_Device);
    m_Gui.VertexDataPack.destory(m_Device);
    
    size_t NumVertex = 24; // 12 edges
    m_Gui.VertexNum = NumVertex;
    VkDeviceSize BufferSize = sizeof(SSimplePointData) * NumVertex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    static const std::array<glm::vec3, 8> Vertices =
    {
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Min.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Min.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Max.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Max.y, vBoundingBox.Min.z),
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Min.y, vBoundingBox.Max.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Min.y, vBoundingBox.Max.z),
        glm::vec3(vBoundingBox.Max.x, vBoundingBox.Max.y, vBoundingBox.Max.z),
        glm::vec3(vBoundingBox.Min.x, vBoundingBox.Max.y, vBoundingBox.Max.z),
    };

    static const std::array<glm::vec3, 24> Edges =
    {
        Vertices[0], Vertices[1],  Vertices[1], Vertices[2],  Vertices[2], Vertices[3],  Vertices[3], Vertices[0],
        Vertices[4], Vertices[5],  Vertices[5], Vertices[6],  Vertices[6], Vertices[7],  Vertices[7], Vertices[4],
        Vertices[0], Vertices[4],  Vertices[1], Vertices[5],  Vertices[2], Vertices[6],  Vertices[3], Vertices[7]
    };
    memcpy(reinterpret_cast<char*>(pData), Edges.data(), sizeof(glm::vec3) * Edges.size());
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Gui.VertexDataPack.Buffer, m_Gui.VertexDataPack.Memory);

    __copyBuffer(StagingBuffer, m_Gui.VertexDataPack.Buffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);

    __recordGuiCommandBuffers();
}

void CVulkanRenderer::__recordGuiCommandBuffers()
{
    for (size_t i = 0; i < m_GuiCommandBuffers.size(); ++i)
    {
        VkCommandBufferInheritanceInfo InheritanceInfo = {};
        InheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        InheritanceInfo.renderPass = m_RenderPass;
        InheritanceInfo.subpass = 1;
        InheritanceInfo.framebuffer = m_Framebuffers[i];

        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        CommandBufferBeginInfo.pInheritanceInfo = &InheritanceInfo;

        ck(vkBeginCommandBuffer(m_GuiCommandBuffers[i], &CommandBufferBeginInfo));
        m_PipelineSet.GuiLines.bind(m_GuiCommandBuffers[i], m_LineDescriptorSets[i]);

        VkDeviceSize Offsets[] = { 0 };
        if (m_Gui.VertexNum > 0)
        {
            vkCmdBindVertexBuffers(m_GuiCommandBuffers[i], 0, 1, &m_Gui.VertexDataPack.Buffer, Offsets);
            vkCmdDraw(m_GuiCommandBuffers[i], m_Gui.VertexNum, 1, 0, 0);
        }
        ck(vkEndCommandBuffer(m_GuiCommandBuffers[i]));
    }
}

void CVulkanRenderer::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeList.clear();
    if (m_Scene.BspTree.Nodes.empty()) throw "场景不含BSP数据";

    m_PipelineSet.TrianglesWithDepthTest.bind(m_SceneCommandBuffers[vImageIndex], m_DescriptorSets[vImageIndex]);

    __renderTreeNode(vImageIndex, 0);
    __renderModels(vImageIndex);
}

void CVulkanRenderer::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    SPushConstant PushConstant = {};
    PushConstant.Opacity = 1.0f;

    if (vNodeIndex >= m_Scene.BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t LeafIndex = vNodeIndex - m_Scene.BspTree.NodeNum;
        for (size_t ObjectIndex : m_Scene.BspTree.LeafIndexToObjectIndices.at(LeafIndex))
        {
            if (!m_AreObjectsVisable[ObjectIndex]) continue;

            m_RenderNodeList.emplace_back(ObjectIndex);
            PushConstant.UseLightmap = m_Scene.Objects[ObjectIndex]->HasLightmap;
            m_PipelineSet.TrianglesWithDepthTest.pushConstant<SPushConstant>(m_SceneCommandBuffers[vImageIndex], VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
            __recordObjectRenderCommand(vImageIndex, ObjectIndex);
        }
    }
    else
    {
        const SBspTreeNode& Node = m_Scene.BspTree.Nodes[vNodeIndex];
        glm::vec3 CameraPos = m_pCamera->getPos();
        if (Node.isPointFrontOfPlane(CameraPos))
        {
            __renderTreeNode(vImageIndex, Node.Back.value());
            __renderTreeNode(vImageIndex, Node.Front.value());
        }
        else
        {
            __renderTreeNode(vImageIndex, Node.Front.value());
            __renderTreeNode(vImageIndex, Node.Back.value());
        }
    }
}

void CVulkanRenderer::__renderModels(uint32_t vImageIndex)
{
    auto [OpaqueSequence, TranparentSequence] = __sortModelRenderSequence();

    m_PipelineSet.TrianglesWithDepthTest.bind(m_SceneCommandBuffers[vImageIndex], m_DescriptorSets[vImageIndex]);
    for(size_t ModelIndex : OpaqueSequence)
        __renderModel(vImageIndex, ModelIndex);

    m_PipelineSet.TrianglesWithBlend.bind(m_SceneCommandBuffers[vImageIndex], m_DescriptorSets[vImageIndex]);
    for (size_t ModelIndex : TranparentSequence)
        __renderModel(vImageIndex, ModelIndex);
}

void CVulkanRenderer::__renderModel(uint32_t vImageIndex, size_t vModelIndex)
{
    _ASSERTE(vModelIndex < m_Scene.BspTree.ModelInfos.size());

    const SModelInfo& ModelInfo = m_Scene.BspTree.ModelInfos[vModelIndex];
    SPushConstant PushConstant = {};
    PushConstant.Opacity = ModelInfo.Opacity;
    std::vector<size_t> ObjectIndices = m_Scene.BspTree.ModelIndexToObjectIndex[vModelIndex];
    for (size_t ObjectIndex : ObjectIndices)
    {
        if (!m_AreObjectsVisable[ObjectIndex]) continue;

        PushConstant.UseLightmap = m_Scene.Objects[ObjectIndex]->HasLightmap;
        m_PipelineSet.TrianglesWithBlend.pushConstant<SPushConstant>(m_SceneCommandBuffers[vImageIndex], VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
        __recordObjectRenderCommand(vImageIndex, ObjectIndex);
    }
}

void CVulkanRenderer::rerecordCommand()
{
    m_RerecordCommand += m_SceneCommandBuffers.size();
}

std::shared_ptr<CCamera> CVulkanRenderer::getCamera()
{
    return m_pCamera;
}

void CVulkanRenderer::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    _ASSERTE(vObjectIndex >= 0 && vObjectIndex < m_Scene.Objects.size());
    std::shared_ptr<S3DObject> pObject = m_Scene.Objects[vObjectIndex];
    SObjectDataPosition DataPosition = m_ObjectDataPositions[vObjectIndex];
    vkCmdSetDepthBias(m_SceneCommandBuffers[vImageIndex], static_cast<float>(vObjectIndex) / m_Scene.Objects.size(), 0, 0);
    if (pObject->DataType == E3DObjectDataType::INDEXED_TRIAGNLE_LIST)
        vkCmdDrawIndexed(m_SceneCommandBuffers[vImageIndex], DataPosition.Size, 1, DataPosition.Offset, 0, 0);
    else if (pObject->DataType == E3DObjectDataType::TRIAGNLE_LIST)
        vkCmdDraw(m_SceneCommandBuffers[vImageIndex], DataPosition.Size, 1, DataPosition.Offset, 0);
    else if (pObject->DataType == E3DObjectDataType::TRIAGNLE_STRIP_LIST)
        vkCmdDraw(m_SceneCommandBuffers[vImageIndex], DataPosition.Size, 1, DataPosition.Offset, 0);
    else
        throw std::runtime_error(u8"物体类型错误");
}

void CVulkanRenderer::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = m_ImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // not present but next render pass

    VkAttachmentDescription DepthAttachment = {};
    DepthAttachment.format = __findDepthFormat();
    DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1;
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDependency, 2> SubpassDependencies = {};
    SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependencies[0].dstSubpass = 0;
    SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].srcAccessMask = 0;
    SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    SubpassDependencies[1].srcSubpass = 0;
    SubpassDependencies[1].dstSubpass = 1;
    SubpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[1].srcAccessMask = 0;
    SubpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColorAttachmentRef;
    SubpassDesc.pDepthStencilAttachment = &DepthAttachmentRef;

    std::vector<VkSubpassDescription> SubpassDescs = { SubpassDesc, SubpassDesc };

    std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment, DepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(SubpassDescs.size());
    RenderPassInfo.pSubpasses = SubpassDescs.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(SubpassDependencies.size());
    RenderPassInfo.pDependencies = SubpassDependencies.data();

    ck(vkCreateRenderPass(m_Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CVulkanRenderer::__createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding UboVertBinding = {};
    UboVertBinding.binding = 0;
    UboVertBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboVertBinding.descriptorCount = 1;
    UboVertBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding UboFragBinding = {};
    UboFragBinding.binding = 1;
    UboFragBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboFragBinding.descriptorCount = 1;
    UboFragBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding SamplerBinding = {};
    SamplerBinding.binding = 2;
    SamplerBinding.descriptorCount = 1;
    SamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    SamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    SamplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding TextureBinding = {};
    TextureBinding.binding = 3;
    TextureBinding.descriptorCount = m_MaxTextureNum;
    TextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    TextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    TextureBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding LightmapBinding = {};
    LightmapBinding.binding = 4;
    LightmapBinding.descriptorCount = 1;
    LightmapBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    LightmapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    LightmapBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> Bindings = 
    {
        UboVertBinding,
        UboFragBinding,
        SamplerBinding,
        TextureBinding,
        LightmapBinding
    };
    VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
    LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutInfo.pBindings = Bindings.data();

    ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void CVulkanRenderer::__createSkyDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding UboVertBinding = {};
    UboVertBinding.binding = 0;
    UboVertBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboVertBinding.descriptorCount = 1;
    UboVertBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding UboFragBinding = {};
    UboFragBinding.binding = 1;
    UboFragBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboFragBinding.descriptorCount = 1;
    UboFragBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding CombinedSamplerBinding = {};
    CombinedSamplerBinding.binding = 2;
    CombinedSamplerBinding.descriptorCount = 1;
    CombinedSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    CombinedSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    CombinedSamplerBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> Bindings =
    {
        UboVertBinding,
        UboFragBinding,
        CombinedSamplerBinding,
    };
    VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
    LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutInfo.pBindings = Bindings.data();

    ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_SkyDescriptorSetLayout));
}

void CVulkanRenderer::__createLineDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding UboVertBinding = {};
    UboVertBinding.binding = 0;
    UboVertBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UboVertBinding.descriptorCount = 1;
    UboVertBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> Bindings =
    {
        UboVertBinding
    };
    VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
    LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
    LayoutInfo.pBindings = Bindings.data();

    ck(vkCreateDescriptorSetLayout(m_Device, &LayoutInfo, nullptr, &m_LineDescriptorSetLayout));
}

void CVulkanRenderer::__createGraphicsPipelines()
{
    __createSkyPipeline();
    __createDepthTestPipeline();
    __createBlendPipeline();
    __createGuiLinesPipeline();
}

void CVulkanRenderer::__createSkyPipeline()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_FALSE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &ColorBlendAttachment;

    m_PipelineSet.TrianglesSky.create(
        m_Device,
        m_RenderPass,
        SSimplePointData::getBindingDescription(),
        SSimplePointData::getAttributeDescriptions(),
        m_Extent,
        m_SkyDescriptorSetLayout,
        DepthStencilInfo,
        ColorBlendInfo
    );
}

void CVulkanRenderer::__createDepthTestPipeline()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &ColorBlendAttachment;

    std::vector<VkDynamicState> EnabledDynamicStates =
    {
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };

    VkPipelineDynamicStateCreateInfo DynamicStateInfo = {};
    DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(EnabledDynamicStates.size());
    DynamicStateInfo.pDynamicStates = EnabledDynamicStates.data();

    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    m_PipelineSet.TrianglesWithDepthTest.create(
        m_Device,
        m_RenderPass,
        SPointData::getBindingDescription(),
        SPointData::getAttributeDescriptions(),
        m_Extent,
        m_DescriptorSetLayout,
        DepthStencilInfo,
        ColorBlendInfo,
        0,
        DynamicStateInfo,
        PushConstantInfo
    );
}

void CVulkanRenderer::__createBlendPipeline()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    // result color = source color * source alpha + old color * (1 - source color)
    // result alpha = source alpha
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_TRUE;
    ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &ColorBlendAttachment;

    std::vector<VkDynamicState> EnabledDynamicStates =
    {
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };

    VkPipelineDynamicStateCreateInfo DynamicStateInfo = {};
    DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(EnabledDynamicStates.size());
    DynamicStateInfo.pDynamicStates = EnabledDynamicStates.data();

    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    m_PipelineSet.TrianglesWithBlend.create(
        m_Device,
        m_RenderPass,
        SPointData::getBindingDescription(),
        SPointData::getAttributeDescriptions(),
        m_Extent,
        m_DescriptorSetLayout,
        DepthStencilInfo,
        ColorBlendInfo,
        0,
        DynamicStateInfo,
        PushConstantInfo
    );
}

void CVulkanRenderer::__createGuiLinesPipeline()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
    ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendInfo.logicOpEnable = VK_FALSE;
    ColorBlendInfo.attachmentCount = 1;
    ColorBlendInfo.pAttachments = &ColorBlendAttachment;

    m_PipelineSet.GuiLines.create(
        m_Device,
        m_RenderPass,
        SSimplePointData::getBindingDescription(),
        SSimplePointData::getAttributeDescriptions(),
        m_Extent,
        m_LineDescriptorSetLayout,
        DepthStencilInfo,
        ColorBlendInfo,
        1
    );
}

void CVulkanRenderer::__createCommandPool()
{
    VkCommandPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    PoolInfo.queueFamilyIndex = m_GraphicsQueueIndex;

    ck(vkCreateCommandPool(m_Device, &PoolInfo, nullptr, &m_CommandPool));
}

void CVulkanRenderer::__createDepthResources()
{
    VkFormat DepthFormat = __findDepthFormat();

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = m_Extent.width;
    ImageInfo.extent.height = m_Extent.height;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = DepthFormat;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    __createImage(ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImagePack.Image, m_DepthImagePack.Memory);
    m_DepthImagePack.ImageView = Common::createImageView(m_Device, m_DepthImagePack.Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    __transitionImageLayout(m_DepthImagePack.Image, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void CVulkanRenderer::__createFramebuffers()
{
    m_Framebuffers.resize(m_NumSwapchainImage);
    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        std::array<VkImageView, 2> Attachments =
        {
            m_ImageViews[i],
            m_DepthImagePack.ImageView
        };

        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = m_RenderPass;
        FramebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        FramebufferInfo.pAttachments = Attachments.data();
        FramebufferInfo.width = m_Extent.width;
        FramebufferInfo.height = m_Extent.height;
        FramebufferInfo.layers = 1;

        ck(vkCreateFramebuffer(m_Device, &FramebufferInfo, nullptr, &m_Framebuffers[i]));
    }
}

void CVulkanRenderer::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImagePacks.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            std::shared_ptr<CIOImage> pImage = m_Scene.TexImages[i];
            __createImageFromIOImage(pImage, m_TextureImagePacks[i].Image, m_TextureImagePacks[i].Memory);
        }
    }
}

void CVulkanRenderer::__createTextureImageViews()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        _ASSERTE(m_TextureImagePacks.size() == NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
            m_TextureImagePacks[i].ImageView = Common::createImageView(m_Device, m_TextureImagePacks[i].Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CVulkanRenderer::__createLightmapImage()
{
    if (m_Scene.UseLightmap)
    {
        std::shared_ptr<CIOImage> pCombinedLightmapImage = m_Scene.pLightmap->getCombinedLightmap();
        __createImageFromIOImage(pCombinedLightmapImage, m_LightmapImagePack.Image, m_LightmapImagePack.Memory);
    }
}

void CVulkanRenderer::__createLightmapImageView()
{
    if (m_Scene.UseLightmap)
    {
        m_LightmapImagePack.ImageView = Common::createImageView(m_Device, m_LightmapImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void CVulkanRenderer::__createTextureSampler()
{
    VkPhysicalDeviceProperties Properties = {};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &Properties);

    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerInfo.magFilter = VK_FILTER_LINEAR;
    SamplerInfo.minFilter = VK_FILTER_LINEAR;
    SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = Properties.limits.maxSamplerAnisotropy;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.0f;
    SamplerInfo.minLod = 0.0f;
    SamplerInfo.maxLod = 0.0f;

    ck(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));
}

void CVulkanRenderer::__createVertexBuffer()
{
    size_t NumVertex = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
        NumVertex += pObject->Vertices.size();
    if (NumVertex == 0)
    {
        globalLog(u8"没有顶点数据，跳过索引缓存创建");
        return;
    }

    VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
    {
        std::vector<SPointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SPointData) * pObject->Vertices.size();
        memcpy(reinterpret_cast<char*>(pData)+ Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);

    __copyBuffer(StagingBuffer, m_VertexBufferPack.Buffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__createIndexBuffer()
{
    size_t NumIndex = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
        NumIndex += pObject->Indices.size();

    if (NumIndex == 0)
    {
        globalLog(u8"没有索引数据，跳过索引缓存创建");
        return;
    }

    VkDeviceSize BufferSize = sizeof(uint32_t) * NumIndex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_Scene.Objects)
    {
        size_t IndexOffset = Offset / sizeof(uint32_t);
        std::vector<uint32_t> Indices = pObject->Indices;
        for (uint32_t& Index : Indices)
            Index += IndexOffset;
        size_t SubBufferSize = sizeof(uint32_t) * Indices.size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, Indices.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vkUnmapMemory(m_Device, StagingBufferMemory);

    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBufferPack.Buffer, m_IndexBufferPack.Memory);

    __copyBuffer(StagingBuffer, m_IndexBufferPack.Buffer, BufferSize);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__createUniformBuffers()
{
    VkDeviceSize BufferSize = sizeof(SUniformBufferObjectVert);
    m_VertUniformBufferPacks.resize(m_NumSwapchainImage);
    m_FragUniformBufferPacks.resize(m_NumSwapchainImage);

    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        __createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPacks[i].Buffer, m_VertUniformBufferPacks[i].Memory);
        __createBuffer(BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPacks[i].Buffer, m_FragUniformBufferPacks[i].Memory);
    }
}

void CVulkanRenderer::__createDescriptorPool()
{
    std::vector<VkDescriptorPoolSize> PoolSizes =
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_NumSwapchainImage) },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_NumSwapchainImage) },
        { VK_DESCRIPTOR_TYPE_SAMPLER, static_cast<uint32_t>(m_NumSwapchainImage) },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(m_NumSwapchainImage * m_MaxTextureNum) },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(m_NumSwapchainImage) },

        // sky box
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_NumSwapchainImage) }, // vert
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_NumSwapchainImage) }, // frag
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_NumSwapchainImage) }, // combined sampler

        // GUI lines
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_NumSwapchainImage) }, // vert
    };

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolInfo.pPoolSizes = PoolSizes.data();
    PoolInfo.maxSets = static_cast<uint32_t>(m_NumSwapchainImage) * 3;

    ck(vkCreateDescriptorPool(m_Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CVulkanRenderer::__createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> Layouts(m_NumSwapchainImage, m_DescriptorSetLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = m_DescriptorPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_NumSwapchainImage);
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    m_DescriptorSets.resize(m_NumSwapchainImage);
    ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_DescriptorSets.data()));
}

void CVulkanRenderer::__createSkyDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> Layouts(m_NumSwapchainImage, m_SkyDescriptorSetLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = m_DescriptorPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_NumSwapchainImage);
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    m_SkyDescriptorSets.resize(m_NumSwapchainImage);
    ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_SkyDescriptorSets.data()));
}

void CVulkanRenderer::__createLineDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> Layouts(m_NumSwapchainImage, m_LineDescriptorSetLayout);

    VkDescriptorSetAllocateInfo DescSetAllocInfo = {};
    DescSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescSetAllocInfo.descriptorPool = m_DescriptorPool;
    DescSetAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_NumSwapchainImage);
    DescSetAllocInfo.pSetLayouts = Layouts.data();

    m_LineDescriptorSets.resize(m_NumSwapchainImage);
    ck(vkAllocateDescriptorSets(m_Device, &DescSetAllocInfo, m_LineDescriptorSets.data()));
}

void CVulkanRenderer::__updateDescriptorSets()
{
    for (size_t i = 0; i < m_DescriptorSets.size(); ++i)
    {
        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferPacks[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUniformBufferObjectFrag);

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_TextureSampler;

        std::vector<VkWriteDescriptorSet> DescriptorWrites;

        VkWriteDescriptorSet VertBufferDescriptorWrite = {};
        VertBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        VertBufferDescriptorWrite.dstSet = m_DescriptorSets[i];
        VertBufferDescriptorWrite.dstBinding = 0;
        VertBufferDescriptorWrite.dstArrayElement = 0;
        VertBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        VertBufferDescriptorWrite.descriptorCount = 1;
        VertBufferDescriptorWrite.pBufferInfo = &VertBufferInfo;
        DescriptorWrites.emplace_back(VertBufferDescriptorWrite);

        VkWriteDescriptorSet FragBufferDescriptorWrite = {};
        FragBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        FragBufferDescriptorWrite.dstSet = m_DescriptorSets[i];
        FragBufferDescriptorWrite.dstBinding = 1;
        FragBufferDescriptorWrite.dstArrayElement = 0;
        FragBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        FragBufferDescriptorWrite.descriptorCount = 1;
        FragBufferDescriptorWrite.pBufferInfo = &FragBufferInfo;
        DescriptorWrites.emplace_back(FragBufferDescriptorWrite);

        VkWriteDescriptorSet SamplerDescriptorWrite = {};
        SamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        SamplerDescriptorWrite.dstSet = m_DescriptorSets[i];
        SamplerDescriptorWrite.dstBinding = 2;
        SamplerDescriptorWrite.dstArrayElement = 0;
        SamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        SamplerDescriptorWrite.descriptorCount = 1;
        SamplerDescriptorWrite.pImageInfo = &SamplerInfo;
        DescriptorWrites.emplace_back(SamplerDescriptorWrite);

        const size_t NumTexture = __getActualTextureNum();
        std::vector<VkDescriptorImageInfo> TexImageInfos;
        if (NumTexture > 0)
        {
            TexImageInfos.resize(m_MaxTextureNum);
            for (size_t i = 0; i < m_MaxTextureNum; ++i)
            {
                // for unused element, fill like the first one (weird method but avoid validationwarning)
                if (i >= NumTexture)
                {
                    TexImageInfos[i] = TexImageInfos[0];
                }
                else
                {
                    TexImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    TexImageInfos[i].imageView = m_TextureImagePacks[i].ImageView;
                    TexImageInfos[i].sampler = VK_NULL_HANDLE;
                }
            }

            VkWriteDescriptorSet TexturesDescriptorWrite = {};
            TexturesDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            TexturesDescriptorWrite.dstSet = m_DescriptorSets[i];
            TexturesDescriptorWrite.dstBinding = 3;
            TexturesDescriptorWrite.dstArrayElement = 0;
            TexturesDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            TexturesDescriptorWrite.descriptorCount = static_cast<uint32_t>(TexImageInfos.size());
            TexturesDescriptorWrite.pImageInfo = TexImageInfos.data();
            DescriptorWrites.emplace_back(TexturesDescriptorWrite);
        }
        
        VkDescriptorImageInfo LightmapImageInfo = {};
        LightmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        LightmapImageInfo.imageView = m_Scene.UseLightmap ?m_LightmapImagePack.ImageView :m_PlaceholderImagePack.ImageView;
        LightmapImageInfo.sampler = VK_NULL_HANDLE;

        VkWriteDescriptorSet LightmapsDescriptorWrite = {};
        LightmapsDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        LightmapsDescriptorWrite.dstSet = m_DescriptorSets[i];
        LightmapsDescriptorWrite.dstBinding = 4;
        LightmapsDescriptorWrite.dstArrayElement = 0;
        LightmapsDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        LightmapsDescriptorWrite.descriptorCount = 1;
        LightmapsDescriptorWrite.pImageInfo = &LightmapImageInfo;
        DescriptorWrites.emplace_back(LightmapsDescriptorWrite);

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
    }
}

void CVulkanRenderer::__updateSkyDescriptorSets()
{
    for (size_t i = 0; i < m_SkyDescriptorSets.size(); ++i)
    {
        std::vector<VkWriteDescriptorSet> DescriptorWrites;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_SkyBox.VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SSkyUniformBufferObjectVert);

        VkWriteDescriptorSet VertBufferDescriptorWrite = {};
        VertBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        VertBufferDescriptorWrite.dstSet = m_SkyDescriptorSets[i];
        VertBufferDescriptorWrite.dstBinding = 0;
        VertBufferDescriptorWrite.dstArrayElement = 0;
        VertBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        VertBufferDescriptorWrite.descriptorCount = 1;
        VertBufferDescriptorWrite.pBufferInfo = &VertBufferInfo;
        DescriptorWrites.emplace_back(VertBufferDescriptorWrite);

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_SkyBox.FragUniformBufferPacks[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SSkyUniformBufferObjectFrag);

        VkWriteDescriptorSet FragBufferDescriptorWrite = {};
        FragBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        FragBufferDescriptorWrite.dstSet = m_SkyDescriptorSets[i];
        FragBufferDescriptorWrite.dstBinding = 1;
        FragBufferDescriptorWrite.dstArrayElement = 0;
        FragBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        FragBufferDescriptorWrite.descriptorCount = 1;
        FragBufferDescriptorWrite.pBufferInfo = &FragBufferInfo;
        DescriptorWrites.emplace_back(FragBufferDescriptorWrite);

        VkDescriptorImageInfo CombinedSamplerInfo = {};
        CombinedSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        CombinedSamplerInfo.imageView = m_SkyBox.SkyBoxImagePack.ImageView;
        CombinedSamplerInfo.sampler = m_TextureSampler;

        VkWriteDescriptorSet SamplerDescriptorWrite = {};
        SamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        SamplerDescriptorWrite.dstSet = m_SkyDescriptorSets[i];
        SamplerDescriptorWrite.dstBinding = 2;
        SamplerDescriptorWrite.dstArrayElement = 0;
        SamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        SamplerDescriptorWrite.descriptorCount = 1;
        SamplerDescriptorWrite.pImageInfo = &CombinedSamplerInfo;
        DescriptorWrites.emplace_back(SamplerDescriptorWrite);

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
    }
}

void CVulkanRenderer::__updateLineDescriptorSets()
{
    for (size_t i = 0; i < m_LineDescriptorSets.size(); ++i)
    {
        std::vector<VkWriteDescriptorSet> DescriptorWrites;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_Gui.VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SGuiUniformBufferObjectVert);

        VkWriteDescriptorSet VertBufferDescriptorWrite = {};
        VertBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        VertBufferDescriptorWrite.dstSet = m_LineDescriptorSets[i];
        VertBufferDescriptorWrite.dstBinding = 0;
        VertBufferDescriptorWrite.dstArrayElement = 0;
        VertBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        VertBufferDescriptorWrite.descriptorCount = 1;
        VertBufferDescriptorWrite.pBufferInfo = &VertBufferInfo;
        DescriptorWrites.emplace_back(VertBufferDescriptorWrite);

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(DescriptorWrites.size()), DescriptorWrites.data(), 0, nullptr);
    }
}

void CVulkanRenderer::__createCommandBuffers()
{
    m_SceneCommandBuffers.resize(m_NumSwapchainImage);

    VkCommandBufferAllocateInfo CommandBufferAllocInfo = {};
    CommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocInfo.commandPool = m_CommandPool;
    CommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferAllocInfo.commandBufferCount = m_NumSwapchainImage;

    ck(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocInfo, m_SceneCommandBuffers.data()));
}

void CVulkanRenderer::__createGuiCommandBuffers()
{
    m_GuiCommandBuffers.resize(m_NumSwapchainImage);

    VkCommandBufferAllocateInfo CommandBufferAllocInfo = {};
    CommandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferAllocInfo.commandPool = m_CommandPool;
    CommandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    CommandBufferAllocInfo.commandBufferCount = m_NumSwapchainImage;

    ck(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocInfo, m_GuiCommandBuffers.data()));
}

void CVulkanRenderer::__createPlaceholderImage()
{
    uint8_t Pixel = 0;

    auto pMinorImage = std::make_shared<CIOImage>();
    pMinorImage->setImageSize(1, 1);
    pMinorImage->setData(&Pixel);
    __createImageFromIOImage(pMinorImage, m_PlaceholderImagePack.Image, m_PlaceholderImagePack.Memory);
    m_PlaceholderImagePack.ImageView = Common::createImageView(m_Device, m_PlaceholderImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

std::vector<SPointData> CVulkanRenderer::__readPointData(std::shared_ptr<S3DObject> vpObject) const
{
    size_t NumPoint = vpObject->Vertices.size();
    _ASSERTE(NumPoint == vpObject->Colors.size());
    _ASSERTE(NumPoint == vpObject->Normals.size());
    _ASSERTE(NumPoint == vpObject->TexCoords.size());
    _ASSERTE(NumPoint == vpObject->LightmapCoords.size());
    _ASSERTE(NumPoint == vpObject->TexIndices.size());

    std::vector<SPointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = vpObject->Vertices[i];
        PointData[i].Color = vpObject->Colors[i];
        PointData[i].Normal = vpObject->Normals[i];
        PointData[i].TexCoord = vpObject->TexCoords[i];
        PointData[i].LightmapCoord = vpObject->LightmapCoords[i];
        PointData[i].TexIndex = vpObject->TexIndices[i];
    }
    return PointData;
}

VkFormat CVulkanRenderer::__findDepthFormat()
{
    return __findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat CVulkanRenderer::__findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures)
{
    for (VkFormat Format : vCandidates)
    {
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, Format, &Props);

        if (vTiling == VK_IMAGE_TILING_LINEAR && 
            (Props.linearTilingFeatures & vFeatures) == vFeatures)
        {
            return Format;
        }
        else if (vTiling == VK_IMAGE_TILING_OPTIMAL && 
            (Props.optimalTilingFeatures & vFeatures) == vFeatures)
        {
            return Format;
        }
    }

    throw std::runtime_error(u8"未找到适配的vulkan格式");
}

void CVulkanRenderer::__createImage(VkImageCreateInfo vImageInfo, VkMemoryPropertyFlags vProperties, VkImage& voImage, VkDeviceMemory& voImageMemory)
{
    ck(vkCreateImage(m_Device, &vImageInfo, nullptr, &voImage));

    VkMemoryRequirements MemRequirements;
    vkGetImageMemoryRequirements(m_Device, voImage, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voImageMemory));

    ck(vkBindImageMemory(m_Device, voImage, voImageMemory, 0));
}

uint32_t CVulkanRenderer::__findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties)
{
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &MemProperties);
    for (uint32_t i = 0; i < MemProperties.memoryTypeCount; ++i)
    {
        if (vTypeFilter & (1 << i) && 
            (MemProperties.memoryTypes[i].propertyFlags & vProperties))
        {
            return i;
        }
    }

    throw std::runtime_error(u8"未找到合适的存储类别");
}

void CVulkanRenderer::__transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount) {
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkImageMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.oldLayout = vOldLayout;
    Barrier.newLayout = vNewLayout;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.image = vImage;

    if (vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (__hasStencilComponent(vFormat))
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseArrayLayer = 0;
    Barrier.subresourceRange.layerCount = vLayerCount;

    VkPipelineStageFlags SrcStage;
    VkPipelineStageFlags DestStage;

    if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && vNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
        && vNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SrcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (vOldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && vNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        SrcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::runtime_error(u8"不支持该布局转换");
    }

    vkCmdPipelineBarrier(
        CommandBuffer,
        SrcStage, DestStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &Barrier
    );

    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

bool CVulkanRenderer::__hasStencilComponent(VkFormat vFormat) {
    return vFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}

void CVulkanRenderer::__createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory)
{
    _ASSERTE(vSize > 0);
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = vSize;
    BufferInfo.usage = vUsage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    ck(vkCreateBuffer(m_Device, &BufferInfo, nullptr, &voBuffer));

    VkMemoryRequirements MemRequirements;
    vkGetBufferMemoryRequirements(m_Device, voBuffer, &MemRequirements);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemRequirements.size;
    AllocInfo.memoryTypeIndex = __findMemoryType(MemRequirements.memoryTypeBits, vProperties);

    ck(vkAllocateMemory(m_Device, &AllocInfo, nullptr, &voBufferMemory));

    ck(vkBindBufferMemory(m_Device, voBuffer, voBufferMemory, 0));
}

void CVulkanRenderer::__copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize)
{
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkBufferCopy CopyRegion = {};
    CopyRegion.size = vSize;
    vkCmdCopyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, 1, &CopyRegion);

    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

void CVulkanRenderer::__copyBufferToImage(VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight, uint32_t vLayerCount)
{
    VkCommandBuffer CommandBuffer = Common::beginSingleTimeCommands(m_Device, m_CommandPool);

    VkBufferImageCopy Region = {};
    Region.bufferOffset = 0;
    Region.bufferRowLength = 0;
    Region.bufferImageHeight = 0;

    Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = vLayerCount;

    Region.imageOffset = { 0, 0, 0 };
    Region.imageExtent = { static_cast<uint32_t>(vWidth), static_cast<uint32_t>(vHeight), 1 };

    vkCmdCopyBufferToImage(CommandBuffer, vBuffer, vImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
    
    Common::endSingleTimeCommands(m_Device, m_CommandPool, m_GraphicsQueue, CommandBuffer);
}

size_t CVulkanRenderer::__getActualTextureNum()
{
    size_t NumTexture = m_Scene.TexImages.size();
    if (NumTexture > m_MaxTextureNum)
    {
        globalLog(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(m_MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = m_MaxTextureNum;
    }
    return NumTexture;
}

void CVulkanRenderer::__createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, VkImage& voImage, VkDeviceMemory& voImageMemory)
{
    int TexWidth = vpImage->getImageWidth();
    int TexHeight = vpImage->getImageHeight();
    const void* pPixelData = vpImage->getData();

    VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;
    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(DataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pDevData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, DataSize, 0, &pDevData));
    memcpy(pDevData, pPixelData, static_cast<size_t>(DataSize));
    vkUnmapMemory(m_Device, StagingBufferMemory);

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = TexWidth;
    ImageInfo.extent.height = TexHeight;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    __createImage(ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, voImage, voImageMemory);
    __transitionImageLayout(voImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    __copyBufferToImage(StagingBuffer, voImage, TexWidth, TexHeight);
    __transitionImageLayout(voImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
    vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
}

void CVulkanRenderer::__calculateVisiableObjects()
{
    SFrustum Frustum = m_pCamera->getFrustum();

    if ((m_RenderMethod == ERenderMethod::BSP || m_EnableCulling) && m_EnablePVS)
        m_CameraNodeIndex = m_Scene.BspTree.getPointLeaf(m_pCamera->getPos());
    else
        m_CameraNodeIndex = std::nullopt;

    // calculate PVS
    std::vector<bool> PVS;
    if (m_EnablePVS)
    {
        PVS.resize(m_Scene.Objects.size(), true);
        for (size_t i = 0; i < m_Scene.BspTree.LeafNum; ++i)
        {
            if (!m_Scene.BspPvs.isLeafVisiable(m_CameraNodeIndex.value(), i))
            {
                std::vector<size_t> LeafObjectIndices = m_Scene.BspTree.LeafIndexToObjectIndices[i];
                for (size_t LeafObjectIndex : LeafObjectIndices)
                    PVS[LeafObjectIndex] = false;
            }
        }
    }

    m_VisableObjectNum = 0;
    for (size_t i = 0; i < m_Scene.Objects.size(); ++i)
    {
        m_AreObjectsVisable[i] = false;

        if (m_EnableSky && m_Scene.Objects[i]->RenderType == E3DObjectRenderType::SKY)
            continue;
        
        if (m_EnableCulling)
        {
            if (i >= m_Scene.BspTree.NodeNum + m_Scene.BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                ++m_VisableObjectNum;
                continue;
            }

            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!__isObjectInSight(m_Scene.Objects[i], Frustum))
                    continue;

            // PVS culling
            if (m_EnablePVS)
                if (!PVS[i])
                    continue;
            
        }

        m_AreObjectsVisable[i] = true;
        ++m_VisableObjectNum;
    }
}

bool CVulkanRenderer::__isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const
{
    // AABB frustum culling
    const std::array<glm::vec4, 6>& FrustumPlanes = vFrustum.Planes;
    S3DBoundingBox BoundingBox = vpObject->getBoundingBox();
    std::array<glm::vec3, 8> BoundPoints = {};
    for (int i = 0; i < 8; ++i)
    {
        float X = ((i & 1) ? BoundingBox.Min.x : BoundingBox.Max.x);
        float Y = ((i & 2) ? BoundingBox.Min.y : BoundingBox.Max.y);
        float Z = ((i & 4) ? BoundingBox.Min.z : BoundingBox.Max.z);
        BoundPoints[i] = glm::vec3(X, Y, Z);
    }

    // for each frustum plane
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 Normal = glm::vec3(FrustumPlanes[i].x, FrustumPlanes[i].y, FrustumPlanes[i].z);
        float D = FrustumPlanes[i].w;
        // if all of the vertices in bounding is behind this plane, the object should not be drawn
        bool NoDraw = true;
        for (int k = 0; k < 8; ++k)
        {
            if (glm::dot(Normal, BoundPoints[k]) + D > 0)
            {
                NoDraw = false;
                break;
            }
        }
        if (NoDraw) return false;
    }
    return true;
}

std::pair<std::vector<size_t>, std::vector<size_t>> CVulkanRenderer::__sortModelRenderSequence()
{
    std::vector<size_t> OpaqueSequence, TranparentSequence;
    std::vector<std::pair<size_t, float>> TranparentInfoForSort;
    glm::vec3 CameraPos = m_pCamera->getPos();
    for (size_t i = 0; i < m_Scene.BspTree.ModelNum; ++i)
    {
        const SModelInfo& ModelInfo = m_Scene.BspTree.ModelInfos[i];
        if (ModelInfo.IsTransparent)
        {
            // simple sort in distance of camera and object center
            S3DBoundingBox BoundingBox = ModelInfo.BoundingBox;
            glm::vec3 Center = (BoundingBox.Min + BoundingBox.Max) * 0.5f;
            float Distance = glm::distance(Center, CameraPos);
            TranparentInfoForSort.emplace_back(std::make_pair(i, Distance));
        }
        else
        {
            OpaqueSequence.emplace_back(i);
        }
    }

    // simple sort, may cause artifact if objects collapse
    std::sort(
        TranparentInfoForSort.begin(),
        TranparentInfoForSort.end(),
        [](const std::pair<size_t, float>& vA, const std::pair<size_t, float>& vB)->bool
        {
            return vA.second > vB.second;
        }
    );

    for (auto Pair : TranparentInfoForSort)
    {
        TranparentSequence.emplace_back(Pair.first);
    }

    return std::make_pair(OpaqueSequence, TranparentSequence);
}

void CVulkanRenderer::recreate(VkFormat vImageFormat, VkExtent2D vExtent, const std::vector<VkImageView>& vImageViews)
{
    vkDeviceWaitIdle(m_Device);
    __destroyRecreateResources();
    m_ImageFormat = vImageFormat;
    m_Extent = vExtent;
    m_ImageViews = vImageViews;
    __createRecreateResources();
    rerecordCommand();
}

void CVulkanRenderer::update(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
    if (m_EnableSky)
        __updateSkyUniformBuffer(vImageIndex);
    __updateGuiUniformBuffer(vImageIndex);
}

void CVulkanRenderer::__updateUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_Extent.height > 0 && m_Extent.width > 0)
        Aspect = static_cast<float>(m_Extent.width) / m_Extent.height;

    m_pCamera->setAspect(Aspect);
    SUniformBufferObjectVert UBOVert = {};
    UBOVert.Model = glm::mat4(1.0f);
    UBOVert.View = m_pCamera->getViewMat();
    UBOVert.Proj = m_pCamera->getProjMat();

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory);

    SUniformBufferObjectFrag UBOFrag = {};
    UBOFrag.Eye = m_pCamera->getPos();

    ck(vkMapMemory(m_Device, m_FragUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferPacks[vImageIndex].Memory);
}

void CVulkanRenderer::__updateSkyUniformBuffer(uint32_t vImageIndex)
{
    SSkyUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = m_pCamera->getProjMat();
    UBOVert.View = m_pCamera->getViewMat();
    UBOVert.EyePosition = m_pCamera->getPos();

    void* pData;
    ck(vkMapMemory(m_Device, m_SkyBox.VertUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_SkyBox.VertUniformBufferPacks[vImageIndex].Memory);

    SSkyUniformBufferObjectFrag UBOFrag = {};
    glm::vec3 FixUp = glm::normalize(glm::vec3(0.0, 1.0, 0.0));
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    glm::vec3 RotationAxe = glm::cross(Up, FixUp);

    if (RotationAxe.length() == 0)
    {
            UBOFrag.UpCorrection = glm::mat4(1.0);
            if (glm::dot(FixUp, Up) < 0)
            {
                UBOFrag.UpCorrection[0][0] = -1.0;
                UBOFrag.UpCorrection[1][1] = -1.0;
                UBOFrag.UpCorrection[2][2] = -1.0;
            }
    }
    else
    {
        float RotationRad = glm::acos(glm::dot(FixUp, Up));
        UBOFrag.UpCorrection = glm::rotate(glm::mat4(1.0), RotationRad, RotationAxe);
    }

    ck(vkMapMemory(m_Device, m_SkyBox.FragUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_SkyBox.FragUniformBufferPacks[vImageIndex].Memory);
}

void CVulkanRenderer::__updateGuiUniformBuffer(uint32_t vImageIndex)
{
    // line
    SGuiUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = m_pCamera->getProjMat();
    UBOVert.View = m_pCamera->getViewMat();

    void* pData;
    ck(vkMapMemory(m_Device, m_Gui.VertUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_Gui.VertUniformBufferPacks[vImageIndex].Memory);
}

void CVulkanRenderer::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    const VkDeviceSize Offsets[] = { 0 };
    m_PipelineSet.TrianglesSky.bind(m_SceneCommandBuffers[vImageIndex], m_SkyDescriptorSets[vImageIndex]);
    vkCmdBindVertexBuffers(m_SceneCommandBuffers[vImageIndex], 0, 1, &m_SkyBox.VertexDataPack.Buffer, Offsets);
    vkCmdDraw(m_SceneCommandBuffers[vImageIndex], m_SkyBox.VertexNum, 1, 0, 0);
}