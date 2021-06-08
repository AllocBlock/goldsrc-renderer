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

    __createRenderPass(false);

    __createDefaultDescriptorSetLayout();
    __createSkyDescriptorSetLayout();
    __createLineDescriptorSetLayout();

    __createCommandPoolAndBuffers();
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
    __createDefaultDescriptorSets(); // imageview
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

    m_EnableSky = m_EnableSky && m_pScene && m_pScene->UseSkyBox;

    if (m_pScene && m_pScene->UseSkyBox)
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

    m_PlaceholderImagePack.destory(m_Device);
    vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    m_DefaultDescriptor.clear();
    m_SkyDescriptor.clear();
    m_LineDescriptor.clear();
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    m_Command.clear();
    m_TextureSampler = VK_NULL_HANDLE;
    m_RenderPass = VK_NULL_HANDLE;
}

void CVulkanRenderer::loadScene(std::shared_ptr<SScene> vpScene)
{
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
     m_VisableObjectNum = 0;

     vkDeviceWaitIdle(m_Device);
     __destroySceneResources();
     __createSceneResources();
}

VkCommandBuffer CVulkanRenderer::requestCommandBuffer(uint32_t vImageIndex)
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
        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

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
                m_PipelineSet.TrianglesWithDepthTest.bind(CommandBuffer, m_DefaultDescriptor.getDescriptorSet(vImageIndex));
                
                SPushConstant PushConstant;
                PushConstant.Opacity = 1.0f;
                
                for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
                {
                    PushConstant.UseLightmap = m_pScene->Objects[i]->HasLightmap;
                    m_PipelineSet.TrianglesWithDepthTest.pushConstant<SPushConstant>(CommandBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
                    if (m_AreObjectsVisable[i])
                        __recordObjectRenderCommand(vImageIndex, i);
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
   
    m_Gui.addOrUpdateObject("HighlightBoundingBox", std::move(pObject));
    __recordGuiCommandBuffers();
}

void CVulkanRenderer::removeHighlightBoundingBox()
{
    m_Gui.removeObject("HighlightBoundingBox");
    __recordGuiCommandBuffers();
}

void CVulkanRenderer::addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd)
{
    auto pObject = std::make_shared<SGuiObject>();
    pObject->Data = { vStart, vEnd };
    m_Gui.addOrUpdateObject(vName, std::move(pObject));
    __recordGuiCommandBuffers();
}

void CVulkanRenderer::__recordGuiCommandBuffers()
{
    vkDeviceWaitIdle(m_Device);
    m_Gui.VertexDataPack.destory(m_Device);

    size_t NumVertex = 0; // 12 edges
    for (const auto& Pair : m_Gui.NameObjectMap)
    {
        const auto& pObject = Pair.second;
        NumVertex += pObject->Data.size();
    }
    if (NumVertex > 0)
    {
        VkDeviceSize BufferSize = sizeof(SSimplePointData) * NumVertex;

        // TODO: addtional copy is made. Is there better way?
        void* pData = new char[BufferSize];
        size_t Offset = 0;
        for (const auto& Pair : m_Gui.NameObjectMap)
        {
            const auto& pObject = Pair.second;
            size_t DataSize = sizeof(glm::vec3) * pObject->Data.size();
            memcpy(reinterpret_cast<char*>(pData) + Offset, pObject->Data.data(), DataSize);
            Offset += DataSize;
        }
        stageFillBuffer(pData, BufferSize, m_Gui.VertexDataPack);
    }

    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
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

        VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_GuiCommandName, i);
        ck(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
        m_PipelineSet.GuiLines.bind(CommandBuffer, m_LineDescriptor.getDescriptorSet(i));

        VkDeviceSize Offsets[] = { 0 };
        if (NumVertex > 0)
        {
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_Gui.VertexDataPack.Buffer, Offsets);
            vkCmdDraw(CommandBuffer, NumVertex, 1, 0, 0);
        }
        ck(vkEndCommandBuffer(CommandBuffer));
    }

    rerecordCommand();
}

void CVulkanRenderer::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeList.clear();
    if (m_pScene->BspTree.Nodes.empty()) throw "场景不含BSP数据";

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.TrianglesWithDepthTest.bind(CommandBuffer, m_DefaultDescriptor.getDescriptorSet(vImageIndex));

    __renderTreeNode(vImageIndex, 0);
    __renderModels(vImageIndex);
}

void CVulkanRenderer::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    SPushConstant PushConstant = {};
    PushConstant.Opacity = 1.0f;

    if (vNodeIndex >= m_pScene->BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t LeafIndex = vNodeIndex - m_pScene->BspTree.NodeNum;
        for (size_t ObjectIndex : m_pScene->BspTree.LeafIndexToObjectIndices.at(LeafIndex))
        {
            if (!m_AreObjectsVisable[ObjectIndex]) continue;

            m_RenderNodeList.emplace_back(ObjectIndex);
            PushConstant.UseLightmap = m_pScene->Objects[ObjectIndex]->HasLightmap;
            m_PipelineSet.TrianglesWithDepthTest.pushConstant<SPushConstant>(CommandBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
            __recordObjectRenderCommand(vImageIndex, ObjectIndex);
        }
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

    m_PipelineSet.TrianglesWithDepthTest.bind(CommandBuffer, m_DefaultDescriptor.getDescriptorSet(vImageIndex));
    for(size_t ModelIndex : OpaqueSequence)
        __renderModel(vImageIndex, ModelIndex);

    m_PipelineSet.TrianglesWithBlend.bind(CommandBuffer, m_DefaultDescriptor.getDescriptorSet(vImageIndex));
    for (size_t ModelIndex : TranparentSequence)
        __renderModel(vImageIndex, ModelIndex);
}

void CVulkanRenderer::__renderModel(uint32_t vImageIndex, size_t vModelIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vModelIndex < m_pScene->BspTree.ModelInfos.size());

    const SModelInfo& ModelInfo = m_pScene->BspTree.ModelInfos[vModelIndex];
    SPushConstant PushConstant = {};
    PushConstant.Opacity = ModelInfo.Opacity;
    std::vector<size_t> ObjectIndices = m_pScene->BspTree.ModelIndexToObjectIndex[vModelIndex];
    for (size_t ObjectIndex : ObjectIndices)
    {
        if (!m_AreObjectsVisable[ObjectIndex]) continue;

        PushConstant.UseLightmap = m_pScene->Objects[ObjectIndex]->HasLightmap;
        m_PipelineSet.TrianglesWithBlend.pushConstant<SPushConstant>(CommandBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
        __recordObjectRenderCommand(vImageIndex, ObjectIndex);
    }
}

void CVulkanRenderer::rerecordCommand()
{
    m_RerecordCommandTimes += m_NumSwapchainImage;
}

std::shared_ptr<CCamera> CVulkanRenderer::getCamera()
{
    return m_pCamera;
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

void CVulkanRenderer::__createRenderPass(bool vPresentLayout)
{
    VkAttachmentDescription ColorAttachment = {};
    ColorAttachment.format = m_ImageFormat;
    ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ColorAttachment.finalLayout = vPresentLayout ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

void CVulkanRenderer::__createDefaultDescriptorSetLayout()
{
    m_DefaultDescriptor.clear();

    m_DefaultDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_DefaultDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_DefaultDescriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_DefaultDescriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_MaxTextureNum, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_DefaultDescriptor.add("Lightmap", 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_DefaultDescriptor.createLayout(m_Device);
}

void CVulkanRenderer::__createSkyDescriptorSetLayout()
{
    m_SkyDescriptor.clear();

    m_SkyDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_SkyDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_SkyDescriptor.add("CombinedSampler", 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_SkyDescriptor.createLayout(m_Device);
}

void CVulkanRenderer::__createLineDescriptorSetLayout()
{
    m_LineDescriptor.clear();

    m_LineDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

    m_LineDescriptor.createLayout(m_Device);
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
        m_SkyDescriptor.getLayout(),
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
        m_DefaultDescriptor.getLayout(),
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
        m_DefaultDescriptor.getLayout(),
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
        m_LineDescriptor.getLayout(),
        DepthStencilInfo,
        ColorBlendInfo,
        1
    );
}

void CVulkanRenderer::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_Device, ECommandType::RESETTABLE, m_GraphicsQueueIndex);
    m_Command.createBuffers(m_SceneCommandName, m_NumSwapchainImage, ECommandBufferLevel::PRIMARY);
    m_Command.createBuffers(m_GuiCommandName, m_NumSwapchainImage, ECommandBufferLevel::SECONDARY);
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
            std::shared_ptr<CIOImage> pImage = m_pScene->TexImages[i];
            __createImageFromIOImage(pImage, m_TextureImagePacks[i]);
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
    if (m_pScene && m_pScene->UseLightmap)
    {
        std::shared_ptr<CIOImage> pCombinedLightmapImage = m_pScene->pLightmap->getCombinedLightmap();
        __createImageFromIOImage(pCombinedLightmapImage, m_LightmapImagePack);
    }
}

void CVulkanRenderer::__createLightmapImageView()
{
    if (m_pScene && m_pScene->UseLightmap)
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

    VkDeviceSize BufferSize = sizeof(SPointData) * NumVertex;

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
   
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
    size_t Offset = 0;
    for (std::shared_ptr<S3DObject> pObject : m_pScene->Objects)
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

    VkBuffer StagingBuffer;
    VkDeviceMemory StagingBufferMemory;
    __createBuffer(BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

    void* pData;
    ck(vkMapMemory(m_Device, StagingBufferMemory, 0, BufferSize, 0, &pData));
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
    auto DefaultPoolSizes = m_DefaultDescriptor.getPoolSizeSet();
    auto SkyPoolSizes = m_SkyDescriptor.getPoolSizeSet();
    auto LinePoolSizes = m_LineDescriptor.getPoolSizeSet();
    std::vector<VkDescriptorPoolSize> PoolSizes;
    PoolSizes.insert(PoolSizes.end(), DefaultPoolSizes.begin(), DefaultPoolSizes.end());
    PoolSizes.insert(PoolSizes.end(), SkyPoolSizes.begin(), SkyPoolSizes.end());
    PoolSizes.insert(PoolSizes.end(), LinePoolSizes.begin(), LinePoolSizes.end());
    for (auto& PoolSize : PoolSizes)
        PoolSize.descriptorCount *= m_NumSwapchainImage;

    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
    PoolInfo.pPoolSizes = PoolSizes.data();
    PoolInfo.maxSets = static_cast<uint32_t>(m_NumSwapchainImage) * 3;

    ck(vkCreateDescriptorPool(m_Device, &PoolInfo, nullptr, &m_DescriptorPool));
}

void CVulkanRenderer::__createDefaultDescriptorSets()
{
    m_DefaultDescriptor.createDescriptorSetSet(m_DescriptorPool, m_NumSwapchainImage);
}

void CVulkanRenderer::__createSkyDescriptorSets()
{
    m_SkyDescriptor.createDescriptorSetSet(m_DescriptorPool, m_NumSwapchainImage);
}

void CVulkanRenderer::__createLineDescriptorSets()
{
    m_LineDescriptor.createDescriptorSetSet(m_DescriptorPool, m_NumSwapchainImage);
}

void CVulkanRenderer::__updateDescriptorSets()
{
    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo}, {} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferPacks[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUniformBufferObjectFrag);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo}, {} }));

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {SamplerInfo} }));

        const size_t NumTexture = __getActualTextureNum();
        
        std::vector<VkDescriptorImageInfo> TexImageInfoSet(m_MaxTextureNum);
        for (size_t i = 0; i < m_MaxTextureNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                {
                    TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    TexImageInfoSet[i].imageView = m_PlaceholderImagePack.ImageView;
                    TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
                }
                else
                {
                    TexImageInfoSet[i] = TexImageInfoSet[0];
                }
            }
            else
            {
                TexImageInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                TexImageInfoSet[i].imageView = m_TextureImagePacks[i].ImageView;
                TexImageInfoSet[i].sampler = VK_NULL_HANDLE;
            }
        }
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageInfoSet }));
        
        VkDescriptorImageInfo LightmapImageInfo = {};
        LightmapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        LightmapImageInfo.imageView = (m_pScene && m_pScene->UseLightmap) ?m_LightmapImagePack.ImageView : m_PlaceholderImagePack.ImageView;
        LightmapImageInfo.sampler = VK_NULL_HANDLE;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {LightmapImageInfo} }));

        m_DefaultDescriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CVulkanRenderer::__updateSkyDescriptorSets()
{
    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_SkyBox.VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SSkyUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_SkyBox.FragUniformBufferPacks[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SSkyUniformBufferObjectFrag);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo }, {} }));

        VkDescriptorImageInfo CombinedSamplerInfo = {};
        CombinedSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        CombinedSamplerInfo.imageView = m_SkyBox.SkyBoxImagePack.ImageView;
        CombinedSamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {CombinedSamplerInfo} }));

        m_SkyDescriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CVulkanRenderer::__updateLineDescriptorSets()
{
    for (size_t i = 0; i < m_NumSwapchainImage; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_Gui.VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SGuiUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} , {} }));

        m_LineDescriptor.update(i, DescriptorWriteInfoSet);
    }
}

void CVulkanRenderer::__createPlaceholderImage()
{
    uint8_t Pixel = 0;

    auto pMinorImage = std::make_shared<CIOImage>();
    pMinorImage->setImageSize(1, 1);
    pMinorImage->setData(&Pixel);
    __createImageFromIOImage(pMinorImage, m_PlaceholderImagePack);
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
    Common::findMemoryType(m_PhysicalDevice, vTypeFilter, vProperties);
}

void CVulkanRenderer::__transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount) {
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();

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

    m_Command.endSingleTimeBuffer(CommandBuffer);
}

bool CVulkanRenderer::__hasStencilComponent(VkFormat vFormat) {
    return vFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || vFormat == VK_FORMAT_D24_UNORM_S8_UINT;
}

void CVulkanRenderer::__createBuffer(VkDeviceSize vSize, VkBufferUsageFlags vUsage, VkMemoryPropertyFlags vProperties, VkBuffer& voBuffer, VkDeviceMemory& voBufferMemory)
{
    Common::createBuffer(m_PhysicalDevice, m_Device, vSize, vUsage, vProperties, voBuffer, voBufferMemory);
}

void CVulkanRenderer::__copyBuffer(VkBuffer vSrcBuffer, VkBuffer vDstBuffer, VkDeviceSize vSize)
{
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    Common::copyBuffer(CommandBuffer, vSrcBuffer, vDstBuffer, vSize);
    m_Command.endSingleTimeBuffer(CommandBuffer);
}

void CVulkanRenderer::__copyBufferToImage(VkBuffer vBuffer, VkImage vImage, size_t vWidth, size_t vHeight, uint32_t vLayerCount)
{
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();

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
    
    m_Command.endSingleTimeBuffer(CommandBuffer);
}

void CVulkanRenderer::stageFillBuffer(const void* vData, VkDeviceSize vSize, Common::SBufferPack& voTargetBufferPack)
{
    Common::SBufferPack StageBufferPack;
    __createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StageBufferPack.Buffer, StageBufferPack.Memory);

    void* pDevData;
    ck(vkMapMemory(m_Device, StageBufferPack.Memory, 0, vSize, 0, &pDevData));
    memcpy(reinterpret_cast<char*>(pDevData), vData, vSize);
    vkUnmapMemory(m_Device, StageBufferPack.Memory);

    __createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, voTargetBufferPack.Buffer, voTargetBufferPack.Memory);

    __copyBuffer(StageBufferPack.Buffer, voTargetBufferPack.Buffer, vSize);

    StageBufferPack.destory(m_Device);
}

void CVulkanRenderer::stageFillImage(const void* vData, VkDeviceSize vSize, VkImageCreateInfo vImageInfo, Common::SImagePack& voTargetImagePack)
{
    uint32_t Width = vImageInfo.extent.width;
    uint32_t Height = vImageInfo.extent.height;
    uint32_t LayerCount = vImageInfo.arrayLayers;

    Common::SBufferPack StageBufferPack;
    __createBuffer(vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StageBufferPack.Buffer, StageBufferPack.Memory);

    void* pDevData;
    ck(vkMapMemory(m_Device, StageBufferPack.Memory, 0, vSize, 0, &pDevData));
    memcpy(pDevData, vData, vSize);
    vkUnmapMemory(m_Device, StageBufferPack.Memory);

    __createImage(vImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, voTargetImagePack.Image, voTargetImagePack.Memory);
    __transitionImageLayout(voTargetImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, LayerCount);
    __copyBufferToImage(StageBufferPack.Buffer, voTargetImagePack.Image, Width, Height, LayerCount);
    __transitionImageLayout(voTargetImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, LayerCount);

    StageBufferPack.destory(m_Device);
}

size_t CVulkanRenderer::__getActualTextureNum()
{
    size_t NumTexture = m_pScene ? m_pScene->TexImages.size() : 0;
    if (NumTexture > m_MaxTextureNum)
    {
        Common::Log::log(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(m_MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = m_MaxTextureNum;
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

    stageFillImage(pPixelData, DataSize, ImageInfo, voImagePack);
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

    m_VisableObjectNum = 0;
    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        m_AreObjectsVisable[i] = false;

        if (m_EnableSky && m_pScene->Objects[i]->RenderType == E3DObjectRenderType::SKY)
            continue;
        
        if (m_EnableCulling)
        {
            if (i >= m_pScene->BspTree.NodeNum + m_pScene->BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                ++m_VisableObjectNum;
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
        ++m_VisableObjectNum;
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
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    const VkDeviceSize Offsets[] = { 0 };
    m_PipelineSet.TrianglesSky.bind(CommandBuffer, m_SkyDescriptor.getDescriptorSet(vImageIndex));
    vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_SkyBox.VertexDataPack.Buffer, Offsets);
    vkCmdDraw(CommandBuffer, m_SkyBox.VertexNum, 1, 0, 0);
}