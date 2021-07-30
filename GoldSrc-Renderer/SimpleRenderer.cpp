#include "SimpleRenderer.h"
#include "Common.h"
#include "Descriptor.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

void CRendererSceneSimple::_loadSceneV(std::shared_ptr<SScene> vScene)
{
    vkDeviceWaitIdle(m_AppInfo.Device);
    m_pScene = vScene;
    m_ObjectDataPositions.resize(m_pScene->Objects.size());

    size_t IndexOffset = 0;
    size_t VertexOffset = 0;
    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        std::shared_ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[i];
        if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST)
        {
            m_ObjectDataPositions[i].Offset = VertexOffset;
            m_ObjectDataPositions[i].Size = pObject->getVertexArray()->size();
            VertexOffset += m_ObjectDataPositions[i].Size;
        }
        else
            throw std::runtime_error(u8"物体类型错误");
    }

    m_AreObjectsVisable.clear();
    m_AreObjectsVisable.resize(m_pScene->Objects.size(), false);

    __destroySceneResources();
    __createSceneResources();

    rerecordCommand();
}

void CRendererSceneSimple::rerecordCommand()
{
    m_RerecordCommandTimes += m_NumSwapchainImage;
}

void CRendererSceneSimple::_initV()
{
    CRendererBase::_initV();
    m_NumSwapchainImage = m_AppInfo.TargetImageViewSet.size();

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createRecreateResources();

    rerecordCommand();
}

void CRendererSceneSimple::_recreateV()
{
    CRendererBase::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
    rerecordCommand();
}

void CRendererSceneSimple::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CRendererSceneSimple::_destroyV()
{
    __destroyRecreateResources();

    vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
    m_Command.clear();

    CRendererBase::_destroyV();
}

std::vector<VkCommandBuffer> CRendererSceneSimple::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    bool RerecordCommand = false;
    if (m_EnableCulling || m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        if (m_RerecordCommandTimes > 0) --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
        CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        Vulkan::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

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
            m_PipelineSet.Main.bind(CommandBuffer, vImageIndex);
            for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
            {
                if (m_AreObjectsVisable[i])
                {
                    __recordObjectRenderCommand(vImageIndex, i);
                }
            }
        }

        vkCmdEndRenderPass(CommandBuffer);
        Vulkan::checkError(vkEndCommandBuffer(CommandBuffer));
    }
    return { CommandBuffer };
}

void CRendererSceneSimple::__createRecreateResources()
{
    __createGraphicsPipelines(); // extent
    __createDepthResources(); // extent
    __createFramebuffers(); // imageview, extent
    m_PipelineSet.Main.setImageNum(m_NumSwapchainImage);
    m_PipelineSet.Sky.setImageNum(m_NumSwapchainImage);
    __createSceneResources();
}

void CRendererSceneSimple::__destroyRecreateResources()
{
    m_DepthImagePack.destroy(m_AppInfo.Device);

    for (auto& Framebuffer : m_FramebufferSet)
        vkDestroyFramebuffer(m_AppInfo.Device, Framebuffer, nullptr);
    m_FramebufferSet.clear();

    __destroySceneResources();
    m_PipelineSet.destroy();
}

void CRendererSceneSimple::__createSceneResources()
{
    __createTextureImages(); // scene
    __updateDescriptorSets();
    __createVertexBuffer(); // scene
    __createIndexBuffer(); // scene

    m_EnableSky = m_EnableSky && m_pScene && m_pScene->UseSkyBox;

    if (m_pScene && m_pScene->UseSkyBox)
    {
        m_PipelineSet.Sky.setSkyBoxImage(m_pScene->SkyBoxImages);
    }
}

void CRendererSceneSimple::__destroySceneResources()
{
    for (size_t i = 0; i < m_TextureImagePackSet.size(); ++i)
    {
        m_TextureImagePackSet[i].destroy(m_AppInfo.Device);
    }
    m_TextureImagePackSet.clear();

    m_IndexBufferPack.destroy(m_AppInfo.Device);
    m_VertexBufferPack.destroy(m_AppInfo.Device);
}

void CRendererSceneSimple::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vObjectIndex >= 0 && vObjectIndex < m_pScene->Objects.size());
    std::shared_ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[vObjectIndex];
    SObjectDataPosition DataPosition = m_ObjectDataPositions[vObjectIndex];

    uint32_t Size = static_cast<uint32_t>(DataPosition.Size);
    uint32_t Offset = static_cast<uint32_t>(DataPosition.Offset);
    if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::INDEXED_TRIAGNLE_LIST)
        vkCmdDrawIndexed(CommandBuffer, Size, 1, Offset, 0, 0);
    else if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST)
        vkCmdDraw(CommandBuffer, Size, 1, Offset, 0);
    else if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_STRIP_LIST)
        vkCmdDraw(CommandBuffer, Size, 1, Offset, 0);
    else
        throw std::runtime_error(u8"物体类型错误");
}

void CRendererSceneSimple::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = CRendererBase::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = CRendererBase::createAttachmentDescription(m_RenderPassPosBitField, __findDepthFormat(), EImageType::DEPTH);

    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1;
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDependency, 1> SubpassDependencies = {};
    SubpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    SubpassDependencies[0].dstSubpass = 0;
    SubpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].srcAccessMask = 0;
    SubpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    SubpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDescription SubpassDesc = {};
    SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDesc.colorAttachmentCount = 1;
    SubpassDesc.pColorAttachments = &ColorAttachmentRef;
    SubpassDesc.pDepthStencilAttachment = &DepthAttachmentRef;

    std::vector<VkSubpassDescription> SubpassDescs = { SubpassDesc };

    std::array<VkAttachmentDescription, 2> Attachments = { ColorAttachment, DepthAttachment };
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RenderPassInfo.pAttachments = Attachments.data();
    RenderPassInfo.subpassCount = static_cast<uint32_t>(SubpassDescs.size());
    RenderPassInfo.pSubpasses = SubpassDescs.data();
    RenderPassInfo.dependencyCount = static_cast<uint32_t>(SubpassDependencies.size());
    RenderPassInfo.pDependencies = SubpassDependencies.data();

    Vulkan::checkError(vkCreateRenderPass(m_AppInfo.Device, &RenderPassInfo, nullptr, &m_RenderPass));
}

void CRendererSceneSimple::__destroyRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_AppInfo.Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void CRendererSceneSimple::__createGraphicsPipelines()
{
    m_PipelineSet.Sky.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
    m_PipelineSet.Main.create(m_AppInfo.PhysicalDevice, m_AppInfo.Device, m_RenderPass, m_AppInfo.Extent);
}

void CRendererSceneSimple::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.Device, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_SceneCommandName, static_cast<uint32_t>(m_NumSwapchainImage), ECommandBufferLevel::PRIMARY);

    Vulkan::beginSingleTimeBufferFunc_t BeginFunc = [this]() -> VkCommandBuffer
    {
        return m_Command.beginSingleTimeBuffer();
    };
    Vulkan::endSingleTimeBufferFunc_t EndFunc = [this](VkCommandBuffer vCommandBuffer)
    {
        m_Command.endSingleTimeBuffer(vCommandBuffer);
    };
    Vulkan::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void CRendererSceneSimple::__createDepthResources()
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

    Vulkan::createImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImagePack.Image, m_DepthImagePack.Memory);
    m_DepthImagePack.ImageView = Vulkan::createImageView(m_AppInfo.Device, m_DepthImagePack.Image, DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    __transitionImageLayout(m_DepthImagePack.Image, DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void CRendererSceneSimple::__createFramebuffers()
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

        Vulkan::checkError(vkCreateFramebuffer(m_AppInfo.Device, &FramebufferInfo, nullptr, &m_FramebufferSet[i]));
    }
}

void CRendererSceneSimple::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImagePackSet.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            std::shared_ptr<CIOImage> pImage = m_pScene->TexImageSet[i];
            __createImageFromIOImage(pImage, m_TextureImagePackSet[i]);
        }
    }
}

void CRendererSceneSimple::__createVertexBuffer()
{
    size_t NumVertex = 0;
    if (m_pScene)
    {
        for (std::shared_ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
            NumVertex += pObject->getVertexArray()->size();
        if (NumVertex == 0)
        {
            Common::Log::log(u8"没有顶点数据，跳过索引缓存创建");
            return;
        }
    }
    else
        return;

    VkDeviceSize BufferSize = sizeof(SSimplePointData) * NumVertex;
    void* pData = new char[BufferSize];
    size_t Offset = 0;
    for (std::shared_ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
    {
        std::vector<SSimplePointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SSimplePointData) * pObject->getVertexArray()->size();
        memcpy(reinterpret_cast<char*>(pData)+ Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    Vulkan::stageFillBuffer(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pData, BufferSize, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);
    delete[] pData;
}

void CRendererSceneSimple::__createIndexBuffer()
{
    /*size_t NumIndex = 0;
    if (m_pScene)
    {
        for (std::shared_ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
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
    Vulkan::stageFillBuffer(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pData, BufferSize, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);
    delete[] pData;*/
}

void CRendererSceneSimple::__updateDescriptorSets()
{
    std::vector<VkImageView> TextureSet(m_TextureImagePackSet.size());
    for (size_t i = 0; i < m_TextureImagePackSet.size(); ++i)
        TextureSet[i] = m_TextureImagePackSet[i].ImageView;
    m_PipelineSet.Main.updateDescriptorSet(TextureSet);
}

std::vector<SSimplePointData> CRendererSceneSimple::__readPointData(std::shared_ptr<C3DObjectGoldSrc> vpObject) const
{
    auto pVertexArray = vpObject->getVertexArray();
    auto pNormalArray = vpObject->getNormalArray();
    auto pTexCoordArray = vpObject->getTexCoordArray();
    auto pTexIndexArray = vpObject->getTexIndexArray();

    size_t NumPoint = pVertexArray->size();
    _ASSERTE(NumPoint == pNormalArray->size());
    _ASSERTE(NumPoint == pTexCoordArray->size());
    _ASSERTE(NumPoint == pTexIndexArray->size());

    std::vector<SSimplePointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = pVertexArray->get(i);
        PointData[i].Normal = pNormalArray->get(i);
        PointData[i].TexCoord = pTexCoordArray->get(i);
        PointData[i].TexIndex = pTexIndexArray->get(i);
    }
    return PointData;
}

VkFormat CRendererSceneSimple::__findDepthFormat()
{
    return __findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat CRendererSceneSimple::__findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures)
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

void CRendererSceneSimple::__transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount) {
    VkCommandBuffer CommandBuffer = m_Command.beginSingleTimeBuffer();
    Vulkan::transitionImageLayout(CommandBuffer, vImage, vFormat, vOldLayout, vNewLayout, vLayerCount);
    m_Command.endSingleTimeBuffer(CommandBuffer);
}

size_t CRendererSceneSimple::__getActualTextureNum()
{
    size_t NumTexture = m_pScene ? m_pScene->TexImageSet.size() : 0;
    if (NumTexture > CPipelineSimple::MaxTextureNum)
    {
        Common::Log::log(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(CPipelineSimple::MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = CPipelineSimple::MaxTextureNum;
    }
    return NumTexture;
}

void CRendererSceneSimple::__createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, Vulkan::SImagePack& voImagePack)
{
    size_t TexWidth = vpImage->getWidth();
    size_t TexHeight = vpImage->getHeight();
    const void* pPixelData = vpImage->getData();

    VkDeviceSize DataSize = static_cast<uint64_t>(4) * TexWidth * TexHeight;

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = static_cast<uint32_t>(TexWidth);
    ImageInfo.extent.height = static_cast<uint32_t>(TexHeight);
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
    Vulkan::stageFillImage(m_AppInfo.PhysicalDevice, m_AppInfo.Device, pPixelData, DataSize, ImageInfo, voImagePack.Image, voImagePack.Memory);
    m_Command.endSingleTimeBuffer(CommandBuffer);

    voImagePack.ImageView = Vulkan::createImageView(m_AppInfo.Device, voImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CRendererSceneSimple::__calculateVisiableObjects()
{
    if (!m_pScene) return;

    SFrustum Frustum = m_pCamera->getFrustum();

    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        m_AreObjectsVisable[i] = false;

        if (m_pScene->Objects[i]->getMark() == "sky" || m_pScene->Objects[i]->getVertexArray()->empty())
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
        }

        m_AreObjectsVisable[i] = true;
    }
}

bool CRendererSceneSimple::__isObjectInSight(std::shared_ptr<C3DObject> vpObject, const SFrustum& vFrustum) const
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

void CRendererSceneSimple::__updateAllUniformBuffer(uint32_t vImageIndex)
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

    m_PipelineSet.Main.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    if (m_EnableSky)
        m_PipelineSet.Sky.updateUniformBuffer(vImageIndex, View, Proj, EyePos, Up);
}

void CRendererSceneSimple::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.Sky.recordCommand(CommandBuffer, vImageIndex);
}