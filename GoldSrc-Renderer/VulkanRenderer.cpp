#include "VulkanRenderer.h"
#include "Common.h"
#include "Descriptor.h"

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

void CVulkanRenderer::loadScene(std::shared_ptr<SScene> vpScene)
{
    vkDeviceWaitIdle(m_AppInfo.Device);
    m_pScene = vpScene;
    m_ObjectDataPositions.resize(m_pScene->Objects.size());
    if (m_pScene->BspTree.Nodes.empty())
        m_RenderMethod = ERenderMethod::DEFAULT;

    size_t IndexOffset = 0;
    size_t VertexOffset = 0;
    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        std::shared_ptr<S3DObject> pObject = m_pScene->Objects[i];
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
    m_AreObjectsVisable.resize(m_pScene->Objects.size(), false);

    vkDeviceWaitIdle(m_AppInfo.Device);
    __destroySceneResources();
    __createSceneResources();
}

void CVulkanRenderer::setHighlightBoundingBox(S3DBoundingBox vBoundingBox)
{
    auto pObject = std::make_shared<SGuiObject>();

    std::array<glm::vec3, 8> Vertices =
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

    pObject->Data =
    {
        Vertices[0], Vertices[1],  Vertices[1], Vertices[2],  Vertices[2], Vertices[3],  Vertices[3], Vertices[0],
        Vertices[4], Vertices[5],  Vertices[5], Vertices[6],  Vertices[6], Vertices[7],  Vertices[7], Vertices[4],
        Vertices[0], Vertices[4],  Vertices[1], Vertices[5],  Vertices[2], Vertices[6],  Vertices[3], Vertices[7]
    };

    m_PipelineSet.GuiLines.setObject("HighlightBoundingBox", std::move(pObject));
    rerecordCommand();
}

void CVulkanRenderer::removeHighlightBoundingBox()
{
    m_PipelineSet.GuiLines.removeObject("HighlightBoundingBox");
    rerecordCommand();
}

void CVulkanRenderer::addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd)
{
    auto pObject = std::make_shared<SGuiObject>();
    pObject->Data = { vStart, vEnd };
    m_PipelineSet.GuiLines.setObject(vName, std::move(pObject));
    rerecordCommand();
}
void CVulkanRenderer::rerecordCommand()
{
    m_RerecordCommandTimes += m_NumSwapchainImage;
}

std::shared_ptr<CCamera> CVulkanRenderer::getCamera()
{
    return m_pCamera;
}

void CVulkanRenderer::_initV()
{
    CRenderer::_initV();
    m_NumSwapchainImage = m_AppInfo.TargetImageViewSet.size();

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createRecreateResources();

    rerecordCommand();
}

void CVulkanRenderer::_recreateV()
{
    CRenderer::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
    rerecordCommand();
}

void CVulkanRenderer::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CVulkanRenderer::_destroyV()
{
    __destroyRecreateResources();

    vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
    m_Command.clear();

    CRenderer::_destroyV();
}

VkCommandBuffer CVulkanRenderer::_requestCommandBufferV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    bool RerecordCommand = false;
    if (m_RenderMethod == ERenderMethod::BSP || m_EnableCulling || m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        if (m_RerecordCommandTimes > 0) --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        __recordGuiCommandBuffer(vImageIndex);

        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        // init

        std::array<VkClearValue, 2> ClearValues = {};
        ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass = m_RenderPass;
        RenderPassBeginInfo.framebuffer = m_FramebufferSet[vImageIndex];
        RenderPassBeginInfo.renderArea.offset = { 0, 0 };
        RenderPassBeginInfo.renderArea.extent = m_AppInfo.Extent;
        RenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
        RenderPassBeginInfo.pClearValues = ClearValues.data();

        vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (m_EnableSky)
            __recordSkyRenderCommand(vImageIndex);

        VkDeviceSize Offsets[] = { 0 };
        if (m_VertexBufferPack.isValid())
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_VertexBufferPack.Buffer, Offsets);
        if (m_IndexBufferPack.isValid())
            vkCmdBindIndexBuffer(CommandBuffer, m_IndexBufferPack.Buffer, 0, VK_INDEX_TYPE_UINT32);

        if (m_VertexBufferPack.isValid() || m_IndexBufferPack.isValid())
        {
            __calculateVisiableObjects();
            if (m_RenderMethod == ERenderMethod::BSP)
                __renderByBspTree(vImageIndex);
            else
            {
                m_PipelineSet.TrianglesWithDepthTest.bind(CommandBuffer, vImageIndex);

                m_PipelineSet.TrianglesWithDepthTest.setOpacity(CommandBuffer, 1.0f);

                for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
                {
                    bool EnableLightmap = m_pScene->Objects[i]->HasLightmap;
                    m_PipelineSet.TrianglesWithDepthTest.setLightmapState(CommandBuffer, EnableLightmap);
                    if (m_AreObjectsVisable[i])
                    {
                        __recordObjectRenderCommand(vImageIndex, i);
                        m_RenderedObjectSet.insert(i);
                    }
                }
            }
        }

        // 3D GUI层，放置选择框等3D标志元素
        vkCmdNextSubpass(CommandBuffer, VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        VkCommandBuffer GuiCommandBuffer = m_Command.getCommandBuffer(m_GuiCommandName, vImageIndex);
        vkCmdExecuteCommands(CommandBuffer, 1, &GuiCommandBuffer);

        vkCmdEndRenderPass(CommandBuffer);
        ck(vkEndCommandBuffer(CommandBuffer));
    }
    return CommandBuffer;
}

void CVulkanRenderer::__createRecreateResources()
{
    __createGraphicsPipelines(); // extent
    __createDepthResources(); // extent
    __createFramebuffers(); // imageview, extent
    m_PipelineSet.TrianglesWithDepthTest.setImageNum(m_NumSwapchainImage);
    m_PipelineSet.TrianglesWithBlend.setImageNum(m_NumSwapchainImage);
    m_PipelineSet.TrianglesSky.setImageNum(m_NumSwapchainImage);
    m_PipelineSet.GuiLines.setImageNum(m_NumSwapchainImage);
    __createSceneResources();
}

void CVulkanRenderer::__destroyRecreateResources()
{
    m_DepthImagePack.destroy(m_AppInfo.Device);

    for (auto& Framebuffer : m_FramebufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, Framebuffer, nullptr);
    m_FramebufferSet.clear();

    __destroySceneResources();
    m_PipelineSet.destroy();
}

void CVulkanRenderer::__createSceneResources()
{
    __createTextureImages(); // scene
    __createLightmapImage(); // scene
    __updateDescriptorSets();
    __createVertexBuffer(); // scene
    __createIndexBuffer(); // scene

    m_EnableSky = m_EnableSky && m_pScene && m_pScene->UseSkyBox;

    if (m_pScene && m_pScene->UseSkyBox)
    {
        m_PipelineSet.TrianglesSky.setSkyBoxImage(m_pScene->SkyBoxImages);
    }
}

void CVulkanRenderer::__destroySceneResources()
{
    for (size_t i = 0; i < m_TextureImagePackSet.size(); ++i)
    {
        m_TextureImagePackSet[i].destroy(m_AppInfo.Device);
    }
    m_TextureImagePackSet.clear();

    m_LightmapImagePack.destroy(m_AppInfo.Device);
    m_IndexBufferPack.destroy(m_AppInfo.Device);
    m_VertexBufferPack.destroy(m_AppInfo.Device);
}

void CVulkanRenderer::__recordGuiCommandBuffer(uint32_t vImageIndex)
{
    _ASSERTE(vImageIndex < m_NumSwapchainImage);
    VkCommandBufferInheritanceInfo InheritanceInfo = {};
    InheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    InheritanceInfo.renderPass = m_RenderPass;
    InheritanceInfo.subpass = 1;
    InheritanceInfo.framebuffer = m_FramebufferSet[vImageIndex];

    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    CommandBufferBeginInfo.pInheritanceInfo = &InheritanceInfo;

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_GuiCommandName, vImageIndex);
    ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
    m_PipelineSet.GuiLines.recordCommand(CommandBuffer, vImageIndex);
    ck(vkEndCommandBuffer(CommandBuffer));
}

void CVulkanRenderer::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeSet.clear();
    if (m_pScene->BspTree.Nodes.empty()) throw "场景不含BSP数据";

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.TrianglesWithDepthTest.bind(CommandBuffer, vImageIndex);

    m_RenderedObjectSet.clear();
    __renderTreeNode(vImageIndex, 0);
    __renderModels(vImageIndex);
}

void CVulkanRenderer::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    m_PipelineSet.TrianglesWithDepthTest.setOpacity(CommandBuffer, 1.0f);

    if (vNodeIndex >= m_pScene->BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t LeafIndex = vNodeIndex - m_pScene->BspTree.NodeNum;
        bool isLeafVisable = false;
        for (size_t ObjectIndex : m_pScene->BspTree.LeafIndexToObjectIndices.at(LeafIndex))
        {
            if (!m_AreObjectsVisable[ObjectIndex]) continue;
            m_RenderedObjectSet.insert(ObjectIndex);
            isLeafVisable = true;

            bool EnableLightmap = m_pScene->Objects[ObjectIndex]->HasLightmap;
            m_PipelineSet.TrianglesWithDepthTest.setLightmapState(CommandBuffer, EnableLightmap);

            __recordObjectRenderCommand(vImageIndex, ObjectIndex);
        }
        if (isLeafVisable)
            m_RenderNodeSet.insert(LeafIndex);
    }
    else
    {
        const SBspTreeNode& Node = m_pScene->BspTree.Nodes[vNodeIndex];
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
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    auto [OpaqueSequence, TranparentSequence] = __sortModelRenderSequence();

    m_PipelineSet.TrianglesWithDepthTest.bind(CommandBuffer, vImageIndex);
    for(size_t ModelIndex : OpaqueSequence)
        __renderModel(vImageIndex, ModelIndex, false);

    m_PipelineSet.TrianglesWithBlend.bind(CommandBuffer, vImageIndex);
    for (size_t ModelIndex : TranparentSequence)
        __renderModel(vImageIndex, ModelIndex, true);
}

void CVulkanRenderer::__renderModel(uint32_t vImageIndex, size_t vModelIndex, bool vBlend)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vModelIndex < m_pScene->BspTree.ModelInfos.size());

    const SModelInfo& ModelInfo = m_pScene->BspTree.ModelInfos[vModelIndex];

    if (vBlend)
        m_PipelineSet.TrianglesWithBlend.setOpacity(CommandBuffer, ModelInfo.Opacity);
    else
        m_PipelineSet.TrianglesWithDepthTest.setOpacity(CommandBuffer, ModelInfo.Opacity);

    std::vector<size_t> ObjectIndices = m_pScene->BspTree.ModelIndexToObjectIndex[vModelIndex];
    for (size_t ObjectIndex : ObjectIndices)
    {
        if (!m_AreObjectsVisable[ObjectIndex]) continue;

        bool EnableLightmap = m_pScene->Objects[ObjectIndex]->HasLightmap;
        if (vBlend)
            m_PipelineSet.TrianglesWithBlend.setLightmapState(CommandBuffer, EnableLightmap);
        else
            m_PipelineSet.TrianglesWithDepthTest.setLightmapState(CommandBuffer, EnableLightmap);
        __recordObjectRenderCommand(vImageIndex, ObjectIndex);
    }
}

void CVulkanRenderer::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vObjectIndex >= 0 && vObjectIndex < m_pScene->Objects.size());
    std::shared_ptr<S3DObject> pObject = m_pScene->Objects[vObjectIndex];
    SObjectDataPosition DataPosition = m_ObjectDataPositions[vObjectIndex];
    vkCmdSetDepthBias(CommandBuffer, static_cast<float>(vObjectIndex) / m_pScene->Objects.size(), 0, 0);
    if (pObject->DataType == E3DObjectDataType::INDEXED_TRIAGNLE_LIST)
        vkCmdDrawIndexed(CommandBuffer, DataPosition.Size, 1, DataPosition.Offset, 0, 0);
    else if (pObject->DataType == E3DObjectDataType::TRIAGNLE_LIST)
        vkCmdDraw(CommandBuffer, DataPosition.Size, 1, DataPosition.Offset, 0);
    else if (pObject->DataType == E3DObjectDataType::TRIAGNLE_STRIP_LIST)
        vkCmdDraw(CommandBuffer, DataPosition.Size, 1, DataPosition.Offset, 0);
    else
        throw std::runtime_error(u8"物体类型错误");
}

void CVulkanRenderer::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = CRenderer::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = CRenderer::createAttachmentDescription(m_RenderPassPosBitField, __findDepthFormat(), EImageType::DEPTH);

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

    ck(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CVulkanRenderer::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CVulkanRenderer::__createGraphicsPipelines()
{
    m_PipelineSet.TrianglesSky.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
    m_PipelineSet.TrianglesWithDepthTest.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
    m_PipelineSet.TrianglesWithBlend.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
    m_PipelineSet.GuiLines.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device,m_RenderPass, m_AppInfo.Extent, 1);
}

void CVulkanRenderer::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_SceneCommandName, m_NumSwapchainImage, ECommandBufferLevel::PRIMARY);
    m_Command.createBuffers(m_GuiCommandName, m_NumSwapchainImage, ECommandBufferLevel::SECONDARY);

    Common::beginSingleTimeBufferFunc_t BeginFunc = [this]() -> VkCommandBuffer
    {
        return m_Command.beginSingleTimeBuffer();
    };
    Common::endSingleTimeBufferFunc_t EndFunc = [this](VkCommandBuffer vCommandBuffer)
    {
        m_Command.endSingleTimeBuffer(vCommandBuffer);
    };
    Common::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void CVulkanRenderer::__createDepthResources()
{
    VkFormat DepthFormat = __findDepthFormat();

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = m_AppInfo.Extent.width;
    ImageInfo.extent.height = m_AppInfo.Extent.height;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = DepthFormat;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Common::createImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImagePack.Image, m_DepthImagePack.Memory);
    m_DepthImagePack.ImageView = Common::createImageView(m_AppInfo.Device, m_DepthImagePack.Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    __transitionImageLayout(m_DepthImagePack.Image, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void CVulkanRenderer::__createFramebuffers()
{
    m_FramebufferSet.resize(m_NumSwapchainImage);
    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        std::array<VkImageView, 2> Attachments =
        {
            m_AppInfo.TargetImageViewSet[i],
            m_DepthImagePack.ImageView
        };

        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = m_RenderPass;
        FramebufferInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
        FramebufferInfo.pAttachments = Attachments.data();
        FramebufferInfo.width = m_AppInfo.Extent.width;
        FramebufferInfo.height = m_AppInfo.Extent.height;
        FramebufferInfo.layers = 1;

        ck(vkCreateFramebuffer(m_AppInfo.Device, &FramebufferInfo, nullptr, &m_FramebufferSet[i]));
    }
}

void CVulkanRenderer::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImagePackSet.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            std::shared_ptr<CIOImage> pImage = m_pScene->TexImages[i];
            __createImageFromIOImage(pImage, m_TextureImagePackSet[i]);
        }
    }
}

void CVulkanRenderer::__createLightmapImage()
{
    if (m_pScene && m_pScene->UseLightmap)
    {
        std::shared_ptr<CIOImage> pCombinedLightmapImage = m_pScene->pLightmap->getCombinedLightmap();
        __createImageFromIOImage(pCombinedLightmapImage, m_LightmapImagePack);
    }
}

void CVulkanRenderer::__createVertexBuffer()
{
    size_t NumVertex = 0;
    if (m_pScene)
    {
        for (std::shared_ptr<S3DObject> pObject : m_pScene->Objects)
            NumVertex += pObject->Vertices.size();
        if (NumVertex == 0)
        {
            Common::Log::log(u8"没有顶点数据，跳过索引缓存创建");
            return;
        }
    }
    else
        return;

    VkDeviceSize BufferSize = sizeof(SGoldSrcPointData) * NumVertex;
    void* pData = new char[BufferSize];
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_pScene->Objects)
    {
        std::vector<SGoldSrcPointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SGoldSrcPointData) * pObject->Vertices.size();
        memcpy(reinterpret_cast<char*>(pData)+ Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    Common::stageFillBuffer(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pData, BufferSize, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);
    delete[] pData;
}

void CVulkanRenderer::__createIndexBuffer()
{
    size_t NumIndex = 0;
    if (m_pScene)
    {
        for (std::shared_ptr<S3DObject> pObject : m_pScene->Objects)
            NumIndex += pObject->Indices.size();

        if (NumIndex == 0)
        {
            Common::Log::log(u8"没有索引数据，跳过索引缓存创建");
            return;
        }
    }
    else
        return;

    VkDeviceSize BufferSize = sizeof(uint32_t) * NumIndex;
    void* pData = new char[BufferSize];
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_pScene->Objects)
    {
        size_t IndexOffset = Offset / sizeof(uint32_t);
        std::vector<uint32_t> Indices = pObject->Indices;
        for (uint32_t& Index : Indices)
            Index += IndexOffset;
        size_t SubBufferSize = sizeof(uint32_t) * Indices.size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, Indices.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    Common::stageFillBuffer(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pData, BufferSize, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);
    delete[] pData;
}

void CVulkanRenderer::__updateDescriptorSets()
{
    std::vector<VkImageView> TextureSet(m_TextureImagePackSet.size());
    VkImageView Lightmap = (m_pScene && m_pScene->UseLightmap) ? m_LightmapImagePack.ImageView : VK_NULL_HANDLE;
    for (size_t i = 0; i < m_TextureImagePackSet.size(); ++i)
        TextureSet[i] = m_TextureImagePackSet[i].ImageView;
    m_PipelineSet.TrianglesWithDepthTest.updateDescriptorSet(TextureSet, Lightmap);
    m_PipelineSet.TrianglesWithBlend.updateDescriptorSet(TextureSet, Lightmap);
    m_PipelineSet.GuiLines.updateDescriptorSet();
}

std::vector<SGoldSrcPointData> CVulkanRenderer::__readPointData(std::shared_ptr<S3DObject> vpObject) const
{
    size_t NumPoint = vpObject->Vertices.size();
    _ASSERTE(NumPoint == vpObject->Colors.size());
    _ASSERTE(NumPoint == vpObject->Normals.size());
    _ASSERTE(NumPoint == vpObject->TexCoords.size());
    _ASSERTE(NumPoint == vpObject->LightmapCoords.size());
    _ASSERTE(NumPoint == vpObject->TexIndices.size());

    std::vector<SGoldSrcPointData> PointData(NumPoint);
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
        vkGetPhysicalDeviceFormatProperties(m_AppInfo.PhysicalDevice, Format, &Props);

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

uint32_t CVulkanRenderer::__findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties)
{
    return Common::findMemoryType(m_AppInfo.PhysicalDevice, vTypeFilter, vProperties);
}

void CVulkanRenderer::__transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount) {
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    Common::transitionImageLayout(CommandBuffer, vImage, vFormat, vOldLayout, vNewLayout, vLayerCount);
    m_Command.endSingleTimeBuffer(CommandBuffer);
}

size_t CVulkanRenderer::__getActualTextureNum()
{
    size_t NumTexture = m_pScene ? m_pScene->TexImages.size() : 0;
    if (NumTexture > CPipelineDepthTest::MaxTextureNum)
    {
        Common::Log::log(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(CPipelineDepthTest::MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = CPipelineDepthTest::MaxTextureNum;
    }
    return NumTexture;
}

void CVulkanRenderer::__createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, Common::SImagePack& voImagePack)
{
    int TexWidth = vpImage->getImageWidth();
    int TexHeight = vpImage->getImageHeight();
    const void* pPixelData = vpImage->getData();

    VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;

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

    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    Common::stageFillImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pPixelData, DataSize, ImageInfo, voImagePack.Image, voImagePack.Memory);
    m_Command.endSingleTimeBuffer(CommandBuffer);

    voImagePack.ImageView = Common::createImageView(m_AppInfo.Device, voImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CVulkanRenderer::__calculateVisiableObjects()
{
    if (!m_pScene) return;

    SFrustum Frustum = m_pCamera->getFrustum();

    if ((m_RenderMethod == ERenderMethod::BSP || m_EnableCulling) && m_EnablePVS)
        m_CameraNodeIndex = m_pScene->BspTree.getPointLeaf(m_pCamera->getPos());
    else
        m_CameraNodeIndex = std::nullopt;

    // calculate PVS
    std::vector<bool> PVS;
    if (m_EnablePVS)
    {
        PVS.resize(m_pScene->Objects.size(), true);
        for (size_t i = 0; i < m_pScene->BspTree.LeafNum; ++i)
        {
            if (!m_pScene->BspPvs.isLeafVisiable(m_CameraNodeIndex.value(), i))
            {
                std::vector<size_t> LeafObjectIndices = m_pScene->BspTree.LeafIndexToObjectIndices[i];
                for (size_t LeafObjectIndex : LeafObjectIndices)
                    PVS[LeafObjectIndex] = false;
            }
        }
    }

    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        m_AreObjectsVisable[i] = false;

        if (m_pScene->Objects[i]->RenderType == E3DObjectRenderType::SKY || m_pScene->Objects[i]->Vertices.empty())
            continue;
        
        if (m_EnableCulling)
        {
            if (i >= m_pScene->BspTree.NodeNum + m_pScene->BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                continue;
            }

            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!__isObjectInSight(m_pScene->Objects[i], Frustum))
                    continue;

            // PVS culling
            if (m_EnablePVS)
                if (!PVS[i])
                    continue;
            
        }

        m_AreObjectsVisable[i] = true;
    }
}

bool CVulkanRenderer::__isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const
{
    // AABB frustum culling
    const std::array<glm::vec4, 6>& FrustumPlanes = vFrustum.Planes;
    std::optional<S3DBoundingBox> BoundingBox = vpObject->getBoundingBox();
    if (BoundingBox == std::nullopt) return false;

    std::array<glm::vec3, 8> BoundPoints = {};
    for (int i = 0; i < 8; ++i)
    {
        float X = ((i & 1) ? BoundingBox.value().Min.x : BoundingBox.value().Max.x);
        float Y = ((i & 2) ? BoundingBox.value().Min.y : BoundingBox.value().Max.y);
        float Z = ((i & 4) ? BoundingBox.value().Min.z : BoundingBox.value().Max.z);
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
    for (size_t i = 0; i < m_pScene->BspTree.ModelNum; ++i)
    {
        const SModelInfo& ModelInfo = m_pScene->BspTree.ModelInfos[i];
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

void CVulkanRenderer::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_PipelineSet.TrianglesWithDepthTest.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    m_PipelineSet.TrianglesWithBlend.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    if (m_EnableSky)
        m_PipelineSet.TrianglesSky.updateUniformBuffer(vImageIndex, View, Proj, EyePos, Up);
    m_PipelineSet.GuiLines.updateUniformBuffer(vImageIndex, View, Proj);
}

void CVulkanRenderer::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.TrianglesSky.recordCommand(CommandBuffer, vImageIndex);
}