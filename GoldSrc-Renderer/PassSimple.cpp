#include "PassSimple.h"
#include "Common.h"
#include "Descriptor.h"
#include "Function.h"
#include "UserInterface.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

void CSceneSimpleRenderPass::_loadSceneV(ptr<SScene> vScene)
{
    vkDeviceWaitIdle(*m_AppInfo.pDevice);
    m_pScene = vScene;
    m_ObjectDataPositions.resize(m_pScene->Objects.size());

    size_t IndexOffset = 0;
    size_t VertexOffset = 0;
    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[i];
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

void CSceneSimpleRenderPass::rerecordCommand()
{
    m_RerecordCommandTimes += m_AppInfo.ImageNum;
}

void CSceneSimpleRenderPass::_initV()
{
    IRenderPass::_initV();
    m_AppInfo.ImageNum = m_AppInfo.ImageNum;

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createRecreateResources();

    rerecordCommand();
}

CRenderPassPort CSceneSimpleRenderPass::_getPortV()
{
    CRenderPassPort Ports;
    Ports.addOutput("Output", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    return Ports;
}

void CSceneSimpleRenderPass::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
    rerecordCommand();
}

void CSceneSimpleRenderPass::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CSceneSimpleRenderPass::_renderUIV()
{
    if (UI::collapse(u8"渲染器设置"))
    {
        bool SkyRendering = getSkyState();
        UI::toggle(u8"开启天空渲染", SkyRendering);
        setSkyState(SkyRendering);

        bool Culling = getCullingState();
        UI::toggle(u8"开启剔除", Culling);
        setCullingState(Culling);
        Culling = getCullingState();

        if (Culling)
        {
            UI::indent(20.0f);
            bool FrustumCulling = getFrustumCullingState();
            UI::toggle(u8"CPU视锥剔除", FrustumCulling);
            setFrustumCullingState(FrustumCulling);
            UI::unindent();
        }
    }
}

void CSceneSimpleRenderPass::_destroyV()
{
    __destroyRecreateResources();

    m_Command.clear();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CSceneSimpleRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_FramebufferSet.empty() || m_pLink->isUpdated())
    {
        __createFramebuffers();
        m_pLink->setUpdateState(false);
    }

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

        vk::checkError(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        // init
        std::vector<VkClearValue> ClearValueSet(2);
        ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValueSet[1].depthStencil = { 1.0f, 0 };

        begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValueSet);

        if (m_EnableSky)
            __recordSkyRenderCommand(vImageIndex);
        
        bool Valid = true;
        VkDeviceSize Offsets[] = { 0 };
        if (m_pVertexBuffer &&m_pVertexBuffer->isValid())
        {
            VkBuffer VertBuffer = *m_pVertexBuffer;
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        }
        else if (m_pIndexBuffer && m_pIndexBuffer->isValid())
            vkCmdBindIndexBuffer(CommandBuffer, *m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        else
            Valid = false;

        if (Valid)
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

        end();
        vk::checkError(vkEndCommandBuffer(CommandBuffer));
    }
    return { CommandBuffer };
}

void CSceneSimpleRenderPass::__createRecreateResources()
{
    __createGraphicsPipelines(); // extent
    __createDepthResources(); // extent
    m_PipelineSet.Main.setImageNum(m_AppInfo.ImageNum);
    m_PipelineSet.Sky.setImageNum(m_AppInfo.ImageNum);
    __createSceneResources();

    for (int i = 0; i < m_AppInfo.ImageNum; ++i)
        m_pLink->link("Depth", *m_pDepthImage, EPortType::OUTPUT, i);
}

void CSceneSimpleRenderPass::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();

    __destroySceneResources();
    m_PipelineSet.destroy();
}

void CSceneSimpleRenderPass::__createSceneResources()
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

void CSceneSimpleRenderPass::__destroySceneResources()
{
    for (size_t i = 0; i < m_TextureImageSet.size(); ++i)
    {
        m_TextureImageSet[i]->destroy();
    }
    m_TextureImageSet.clear();

    if (m_pIndexBuffer) m_pIndexBuffer->destroy();
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();
}

void CSceneSimpleRenderPass::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vObjectIndex >= 0 && vObjectIndex < m_pScene->Objects.size());
    ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[vObjectIndex];
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

void CSceneSimpleRenderPass::__createRenderPass()
{
    VkAttachmentDescription ColorAttachment = IRenderPass::createAttachmentDescription(m_RenderPassPosBitField, m_AppInfo.ImageFormat, vk::EImageType::COLOR);
    VkAttachmentDescription DepthAttachment = IRenderPass::createAttachmentDescription(m_RenderPassPosBitField, __findDepthFormat(), vk::EImageType::DEPTH);

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

    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &RenderPassInfo, nullptr, _getPtr()));
}

void CSceneSimpleRenderPass::__createGraphicsPipelines()
{
    m_PipelineSet.Sky.create(m_AppInfo.pPhysicalDevice, m_AppInfo.pDevice, get(), m_AppInfo.Extent);
    m_PipelineSet.Main.create(m_AppInfo.pPhysicalDevice, m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CSceneSimpleRenderPass::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE, m_AppInfo.GraphicsQueueIndex);
    m_Command.createBuffers(m_SceneCommandName, static_cast<uint32_t>(m_AppInfo.ImageNum), ECommandBufferLevel::PRIMARY);
}

void CSceneSimpleRenderPass::__createDepthResources()
{
    VkFormat DepthFormat = __findDepthFormat();
    m_pDepthImage = Function::createDepthImage(m_AppInfo.pPhysicalDevice, m_AppInfo.pDevice, m_AppInfo.Extent, NULL, DepthFormat);
}

void CSceneSimpleRenderPass::__createFramebuffers()
{
    m_FramebufferSet.resize(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pLink->getOutput("Output", i),
            *m_pDepthImage
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CSceneSimpleRenderPass::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            m_TextureImageSet[i] = Function::createImageFromIOImage(m_AppInfo.pPhysicalDevice, m_AppInfo.pDevice, m_pScene->TexImageSet[i]);
        }
    }
}

void CSceneSimpleRenderPass::__createVertexBuffer()
{
    size_t NumVertex = 0;
    if (m_pScene)
    {
        for (ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
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
    for (ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
    {
        std::vector<SSimplePointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SSimplePointData) * pObject->getVertexArray()->size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }

    m_pVertexBuffer = make<vk::CBuffer>();
    m_pVertexBuffer->create(m_AppInfo.pPhysicalDevice, m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertexBuffer->stageFill(pData, BufferSize);
    delete[] pData;
}

void CSceneSimpleRenderPass::__createIndexBuffer()
{
    /*size_t NumIndex = 0;
    if (m_pScene)
    {
        for (ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
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
    for (ptr<S3DObject> pObject : m_pScene->Objects)
    {
        size_t IndexOffset = Offset / sizeof(uint32_t);
        std::vector<uint32_t> Indices = pObject->Indices;
        for (uint32_t& Index : Indices)
            Index += IndexOffset;
        size_t SubBufferSize = sizeof(uint32_t) * Indices.size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, Indices.data(), SubBufferSize);
        Offset += SubBufferSize;
    }
    vk::stageFillBuffer(*m_AppInfo.pPhysicalDevice, *m_AppInfo.pDevice, pData, BufferSize, m_VertexBufferPack.Buffer, m_VertexBufferPack.Memory);
    delete[] pData;*/
}

void CSceneSimpleRenderPass::__updateDescriptorSets()
{
    std::vector<VkImageView> TextureSet(m_TextureImageSet.size());
    for (size_t i = 0; i < m_TextureImageSet.size(); ++i)
        TextureSet[i] = *m_TextureImageSet[i];
    m_PipelineSet.Main.updateDescriptorSet(TextureSet);
}

std::vector<SSimplePointData> CSceneSimpleRenderPass::__readPointData(ptr<C3DObjectGoldSrc> vpObject) const
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

VkFormat CSceneSimpleRenderPass::__findDepthFormat()
{
    return __findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat CSceneSimpleRenderPass::__findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures)
{
    for (VkFormat Format : vCandidates)
    {
        VkFormatProperties Props;
        vkGetPhysicalDeviceFormatProperties(*m_AppInfo.pPhysicalDevice, Format, &Props);

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

size_t CSceneSimpleRenderPass::__getActualTextureNum()
{
    size_t NumTexture = m_pScene ? m_pScene->TexImageSet.size() : 0;
    if (NumTexture > CPipelineSimple::MaxTextureNum)
    {
        Common::Log::log(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(CPipelineSimple::MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = CPipelineSimple::MaxTextureNum;
    }
    return NumTexture;
}

void CSceneSimpleRenderPass::__calculateVisiableObjects()
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

bool CSceneSimpleRenderPass::__isObjectInSight(ptr<C3DObject> vpObject, const SFrustum& vFrustum) const
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

void CSceneSimpleRenderPass::__updateAllUniformBuffer(uint32_t vImageIndex)
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

void CSceneSimpleRenderPass::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.Sky.recordCommand(CommandBuffer, vImageIndex);
}