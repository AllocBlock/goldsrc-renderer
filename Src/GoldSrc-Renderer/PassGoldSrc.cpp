#include "PassGoldSrc.h"
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

void CSceneGoldSrcRenderPass::_loadSceneV(ptr<SScene> vScene)
{
    m_AppInfo.pDevice->waitUntilIdle();
    m_pScene = vScene;
    m_ObjectDataPositions.resize(m_pScene->Objects.size());
    if (m_pScene->BspTree.Nodes.empty()) m_EnableBSP = false;
    if (m_pScene->BspPvs.LeafNum == 0) m_EnablePVS = false;

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
}

void CSceneGoldSrcRenderPass::rerecordCommand()
{
    m_RerecordCommandTimes += m_AppInfo.ImageNum;
}

void CSceneGoldSrcRenderPass::_initV()
{
    IRenderPass::_initV();

    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createRecreateResources();

    rerecordCommand();
}

CRenderPassPort CSceneGoldSrcRenderPass::_getPortV()
{
    CRenderPassPort Ports;
    Ports.addOutput("Output", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    return Ports;
}

void CSceneGoldSrcRenderPass::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
    rerecordCommand();
}

void CSceneGoldSrcRenderPass::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CSceneGoldSrcRenderPass::_renderUIV()
{
    if (UI::collapse(u8"仿金源"))
    {
        bool EnableBSP = getBSPState();
        UI::toggle(u8"使用BSP树渲染（支持实体渲染模式）", EnableBSP);
        setBSPState(EnableBSP);

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

            bool PVS = getPVSState();
            UI::toggle(u8"PVS剔除", PVS);
            setPVSState(PVS);
            UI::unindent();
        }

        std::optional<uint32_t> CameraNodeIndex = getCameraNodeIndex();
        if (CameraNodeIndex == std::nullopt)
            UI::text(u8"相机所处节点：-");
        else
            UI::text((u8"相机所处节点：" + std::to_string(CameraNodeIndex.value())).c_str());

        if (!getBSPState())
        {
            std::set<size_t> RenderObjectList = getRenderedObjectSet();
            UI::text((u8"渲染物体数：" + std::to_string(RenderObjectList.size())).c_str());
            if (!RenderObjectList.empty())
            {
                std::string RenderNodeListStr = "";
                for (size_t ObjectIndex : RenderObjectList)
                {
                    RenderNodeListStr += std::to_string(ObjectIndex) + ", ";
                }
                UI::text((u8"渲染物体：" + RenderNodeListStr).c_str(), true);
            }
        }
        else
        {
            std::set<uint32_t> RenderNodeList = getRenderedNodeList();
            UI::text((u8"渲染节点数：" + std::to_string(RenderNodeList.size())).c_str());
            if (!RenderNodeList.empty())
            {
                std::string RenderNodeListStr = "";
                for (uint32_t NodeIndex : RenderNodeList)
                {
                    RenderNodeListStr += std::to_string(NodeIndex) + ", ";
                }
                UI::text((u8"渲染节点：" + RenderNodeListStr).c_str(), true);
            }
        }
    }
}

void CSceneGoldSrcRenderPass::_destroyV()
{
    __destroyRecreateResources();

    m_Command.clear();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CSceneGoldSrcRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_FramebufferSet.empty() || m_pLink->isUpdated())
    {
        __createFramebuffers();
        m_pLink->setUpdateState(false);
    }

    m_RenderedObjectSet.clear();

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    bool RerecordCommand = false;
    if (m_EnableBSP || m_EnableCulling || m_RerecordCommandTimes > 0)
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
        if (m_pVertexBuffer && m_pVertexBuffer->isValid())
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
            if (m_EnableBSP)
                __renderByBspTree(vImageIndex);
            else
            {
                m_PipelineSet.DepthTest.bind(CommandBuffer, vImageIndex);

                m_PipelineSet.DepthTest.setOpacity(CommandBuffer, 1.0f);

                for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
                {
                    bool EnableLightmap = m_pScene->Objects[i]->getLightMapState();
                    m_PipelineSet.DepthTest.setLightmapState(CommandBuffer, EnableLightmap);
                    if (m_AreObjectsVisable[i])
                    {
                        __recordObjectRenderCommand(vImageIndex, i);
                        m_RenderedObjectSet.insert(i);
                    }
                }
            }
        }

        if (m_pScene && !m_pScene->SprSet.empty())
            __recordSpriteRenderCommand(vImageIndex);

        end();
        vk::checkError(vkEndCommandBuffer(CommandBuffer));
    }
    return { CommandBuffer };
}

void CSceneGoldSrcRenderPass::__createRecreateResources()
{
    __createGraphicsPipelines(); // extent
    __createDepthResources(); // extent
    uint32_t ImageNum = m_AppInfo.ImageNum;
    m_PipelineSet.DepthTest.setImageNum(ImageNum);
    m_PipelineSet.BlendTextureAlpha.setImageNum(ImageNum);
    m_PipelineSet.BlendAlphaTest.setImageNum(ImageNum);
    m_PipelineSet.BlendAdditive.setImageNum(ImageNum);
    m_PipelineSet.Sprite.setImageNum(ImageNum);
    m_PipelineSet.Sky.setImageNum(ImageNum);
    __createSceneResources();

    for (int i = 0; i < ImageNum; ++i)
        m_pLink->link("Depth", *m_pDepthImage, EPortType::OUTPUT, i);
}

void CSceneGoldSrcRenderPass::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();

    __destroySceneResources();
    m_PipelineSet.destroy();
}

void CSceneGoldSrcRenderPass::__createSceneResources()
{
    __createTextureImages();
    __createLightmapImage();
    __updateDescriptorSets();
    __createVertexBuffer();
    __createIndexBuffer();

    m_EnableSky = m_EnableSky && m_pScene && m_pScene->UseSkyBox;

    if (m_pScene && m_pScene->UseSkyBox)
    {
        m_PipelineSet.Sky.setSkyBoxImage(m_pScene->SkyBoxImages);
    }

    if (m_pScene && !m_pScene->SprSet.empty())
    {
        m_PipelineSet.Sprite.setSprites(m_pScene->SprSet);
    }

    rerecordCommand();
}

void CSceneGoldSrcRenderPass::__destroySceneResources()
{
    for (size_t i = 0; i < m_TextureImageSet.size(); ++i)
    {
        m_TextureImageSet[i]->destroy();
    }
    m_TextureImageSet.clear();

    if (m_pLightmapImage) m_pLightmapImage->destroy();
    if (m_pIndexBuffer) m_pIndexBuffer->destroy();
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();
}

void CSceneGoldSrcRenderPass::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeSet.clear();
    if (m_pScene->BspTree.Nodes.empty()) throw "场景不含BSP数据";

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.DepthTest.bind(CommandBuffer, vImageIndex);

    __renderTreeNode(vImageIndex, 0);
    __renderPointEntities(vImageIndex);
    __renderModels(vImageIndex);
    __renderSprites(vImageIndex);
}

void CSceneGoldSrcRenderPass::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    m_PipelineSet.DepthTest.setOpacity(CommandBuffer, 1.0f);

    if (vNodeIndex >= m_pScene->BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t LeafIndex = static_cast<uint32_t>(vNodeIndex - m_pScene->BspTree.NodeNum);
        bool isLeafVisable = false;
        for (size_t ObjectIndex : m_pScene->BspTree.LeafIndexToObjectIndices.at(LeafIndex))
        {
            if (!m_AreObjectsVisable[ObjectIndex]) continue;
            m_RenderedObjectSet.insert(ObjectIndex);
            isLeafVisable = true;

            bool EnableLightmap = m_pScene->Objects[ObjectIndex]->getLightMapState();
            m_PipelineSet.DepthTest.setLightmapState(CommandBuffer, EnableLightmap);

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

void CSceneGoldSrcRenderPass::__renderModels(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    // 这里去看了下Xash3D的源码，xash3d/engine/client/gl_rmain.c
    // 它是按照纹理、叠加和发光的顺序绘制
    // 
    // 也对实体进行简单的按距离排序
    auto SortedModelSet = __sortModelRenderSequence();

    for (size_t ModelIndex : SortedModelSet)
    {
        __renderModel(vImageIndex, ModelIndex);
    }
}

void CSceneGoldSrcRenderPass::__renderSprites(uint32_t vImageIndex)
{

}

struct SModelSortInfo
{
    size_t Index;
    EGoldSrcRenderMode RenderMode;
    float Distance;

    int getRenderModePriority() const
    {
        switch (RenderMode)
        {
        case EGoldSrcRenderMode::NORMAL:
        case EGoldSrcRenderMode::SOLID:
            return 0;
        case EGoldSrcRenderMode::COLOR:
        case EGoldSrcRenderMode::TEXTURE:
            return 1;
        case EGoldSrcRenderMode::GLOW:
        case EGoldSrcRenderMode::ADDITIVE:
            return 2;
        default: return -1;
        }
    }
};

std::vector<size_t> CSceneGoldSrcRenderPass::__sortModelRenderSequence()
{
    std::vector<SModelSortInfo> InfoForSort;
    glm::vec3 CameraPos = m_pCamera->getPos();
    for (size_t i = 0; i < m_pScene->BspTree.ModelNum; ++i)
    {
        const SModelInfo& ModelInfo = m_pScene->BspTree.ModelInfos[i];
        S3DBoundingBox BoundingBox = ModelInfo.BoundingBox;
        glm::vec3 Center = (BoundingBox.Min + BoundingBox.Max) * 0.5f;
        float Distance = glm::distance(Center, CameraPos);

        InfoForSort.emplace_back(SModelSortInfo{ i, ModelInfo.RenderMode, Distance });
    }

    // simple sort, may cause artifact if objects collapse
    std::sort(
        InfoForSort.begin(),
        InfoForSort.end(),
        [](const SModelSortInfo& vA, const SModelSortInfo& vB)->bool
        {
            int ModePrioA = vA.getRenderModePriority();
            int ModePrioB = vB.getRenderModePriority();
            if (ModePrioA < ModePrioB) return true;
            else if (ModePrioA == ModePrioB) return vA.Distance > vB.Distance;
            else return false;
        }
    );

    std::vector<size_t> SortedModelSet;
    for (const SModelSortInfo& Info : InfoForSort)
    {
        SortedModelSet.emplace_back(Info.Index);
    }

    return SortedModelSet;
}

void CSceneGoldSrcRenderPass::__renderModel(uint32_t vImageIndex, size_t vModelIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vModelIndex < m_pScene->BspTree.ModelInfos.size());

    const SModelInfo& ModelInfo = m_pScene->BspTree.ModelInfos[vModelIndex];
    CPipelineDepthTest* pPipeline = nullptr;

    switch (ModelInfo.RenderMode)
    {
    case EGoldSrcRenderMode::NORMAL:
    {
        pPipeline = &m_PipelineSet.DepthTest;
        break;
    }
    case EGoldSrcRenderMode::COLOR:
    case EGoldSrcRenderMode::TEXTURE:
    {
        pPipeline = &m_PipelineSet.BlendTextureAlpha;
        break;
    }
    case EGoldSrcRenderMode::SOLID:
    {
        pPipeline = &m_PipelineSet.BlendAlphaTest;
        break;
    }
    case EGoldSrcRenderMode::ADDITIVE:
    case EGoldSrcRenderMode::GLOW:
    {
        pPipeline = &m_PipelineSet.BlendAdditive;
        break;
    }
    default:
        break;
    }

    pPipeline->bind(CommandBuffer, vImageIndex);
    pPipeline->setOpacity(CommandBuffer, ModelInfo.Opacity);

    std::vector<size_t> ObjectIndices = m_pScene->BspTree.ModelIndexToObjectIndex[vModelIndex];
    for (size_t ObjectIndex : ObjectIndices)
    {
        if (!m_AreObjectsVisable[ObjectIndex]) continue;

        bool EnableLightmap = m_pScene->Objects[ObjectIndex]->getLightMapState();
        pPipeline->setLightmapState(CommandBuffer, EnableLightmap);
        __recordObjectRenderCommand(vImageIndex, ObjectIndex);
    }
}

void CSceneGoldSrcRenderPass::__renderPointEntities(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.DepthTest.bind(CommandBuffer, vImageIndex);

    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        if (!m_AreObjectsVisable[i]) continue;
        else if (m_pScene->Objects[i]->getMark() != "point_entity") continue;
        __recordObjectRenderCommand(vImageIndex, i);
    }
}

void CSceneGoldSrcRenderPass::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

    _ASSERTE(vObjectIndex >= 0 && vObjectIndex < m_pScene->Objects.size());
    ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[vObjectIndex];
    SObjectDataPosition DataPosition = m_ObjectDataPositions[vObjectIndex];

    uint32_t Size = static_cast<uint32_t>(DataPosition.Size);
    uint32_t Offset = static_cast<uint32_t>(DataPosition.Offset);
    vkCmdSetDepthBias(CommandBuffer, static_cast<float>(vObjectIndex) / m_pScene->Objects.size(), 0, 0);
    if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::INDEXED_TRIAGNLE_LIST)
        vkCmdDrawIndexed(CommandBuffer, Size, 1, Offset, 0, 0);
    else if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST)
        vkCmdDraw(CommandBuffer, Size, 1, Offset, 0);
    else if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_STRIP_LIST)
        vkCmdDraw(CommandBuffer, Size, 1, Offset, 0);
    else
        throw std::runtime_error(u8"物体类型错误");
}

void CSceneGoldSrcRenderPass::__createRenderPass()
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

void CSceneGoldSrcRenderPass::__createGraphicsPipelines()
{
    VkRenderPass RenderPass = get();
    m_PipelineSet.Sky.create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
    m_PipelineSet.DepthTest.create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
    m_PipelineSet.BlendTextureAlpha.create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
    m_PipelineSet.BlendAlphaTest.create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
    m_PipelineSet.BlendAdditive.create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
    m_PipelineSet.Sprite.create(m_AppInfo.pDevice, RenderPass, m_AppInfo.Extent);
}

void CSceneGoldSrcRenderPass::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_SceneCommandName, static_cast<uint32_t>(m_AppInfo.ImageNum), ECommandBufferLevel::PRIMARY);
}

void CSceneGoldSrcRenderPass::__createDepthResources()
{
    VkFormat DepthFormat = __findDepthFormat();
    m_pDepthImage = Function::createDepthImage(m_AppInfo.pDevice, m_AppInfo.Extent, NULL, DepthFormat);
}

void CSceneGoldSrcRenderPass::__createFramebuffers()
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

void CSceneGoldSrcRenderPass::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            m_TextureImageSet[i] = Function::createImageFromIOImage(m_AppInfo.pDevice, m_pScene->TexImageSet[i]);
        }
    }
}

void CSceneGoldSrcRenderPass::__createLightmapImage()
{
    if (m_pScene && m_pScene->UseLightmap)
    {
        ptr<CIOImage> pCombinedLightmapImage = m_pScene->pLightmap->getCombinedLightmap();
        m_pLightmapImage = Function::createImageFromIOImage(m_AppInfo.pDevice, pCombinedLightmapImage);
    }
}

void CSceneGoldSrcRenderPass::__createVertexBuffer()
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

    VkDeviceSize BufferSize = sizeof(SGoldSrcPointData) * NumVertex;
    void* pData = new char[BufferSize];
    size_t Offset = 0;
    for (ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
    {
        std::vector<SGoldSrcPointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SGoldSrcPointData) * pObject->getVertexArray()->size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }

    m_pVertexBuffer = make<vk::CBuffer>();
    m_pVertexBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertexBuffer->stageFill(pData, BufferSize);
    delete[] pData;
}

void CSceneGoldSrcRenderPass::__createIndexBuffer()
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
    vk::stageFillBuffer(**m_AppInfo.pDevice, pData, BufferSize, *m_VertexBufferPack, m_VertexBufferPack.Memory);
    delete[] pData;*/
}

void CSceneGoldSrcRenderPass::__updateDescriptorSets()
{
    std::vector<VkImageView> TextureSet(m_TextureImageSet.size());
    VkImageView Lightmap = (m_pScene && m_pScene->UseLightmap) ? *m_pLightmapImage : VK_NULL_HANDLE;
    for (size_t i = 0; i < m_TextureImageSet.size(); ++i)
        TextureSet[i] = *m_TextureImageSet[i];
    m_PipelineSet.DepthTest.updateDescriptorSet(TextureSet, Lightmap);
    m_PipelineSet.BlendTextureAlpha.updateDescriptorSet(TextureSet, Lightmap);
    m_PipelineSet.BlendAlphaTest.updateDescriptorSet(TextureSet, Lightmap);
    m_PipelineSet.BlendAdditive.updateDescriptorSet(TextureSet, Lightmap);
}

std::vector<SGoldSrcPointData> CSceneGoldSrcRenderPass::__readPointData(ptr<C3DObjectGoldSrc> vpObject) const
{
    auto pVertexArray = vpObject->getVertexArray();
    auto pColorArray = vpObject->getColorArray();
    auto pNormalArray = vpObject->getNormalArray();
    auto pTexCoordArray = vpObject->getTexCoordArray();
    auto pLightmapCoordArray = vpObject->getLightmapCoordArray();
    auto pTexIndexArray = vpObject->getTexIndexArray();

    size_t NumPoint = pVertexArray->size();
    _ASSERTE(NumPoint == pColorArray->size());
    _ASSERTE(NumPoint == pNormalArray->size());
    _ASSERTE(NumPoint == pTexCoordArray->size());
    _ASSERTE(NumPoint == pLightmapCoordArray->size());
    _ASSERTE(NumPoint == pTexIndexArray->size());

    std::vector<SGoldSrcPointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = pVertexArray->get(i);
        PointData[i].Color = pColorArray->get(i);
        PointData[i].Normal = pNormalArray->get(i);
        PointData[i].TexCoord = pTexCoordArray->get(i);
        PointData[i].LightmapCoord = pLightmapCoordArray->get(i);
        PointData[i].TexIndex = pTexIndexArray->get(i);
    }
    return PointData;
}

VkFormat CSceneGoldSrcRenderPass::__findDepthFormat()
{
    return m_AppInfo.pDevice->getPhysicalDevice()->chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

size_t CSceneGoldSrcRenderPass::__getActualTextureNum()
{
    size_t NumTexture = m_pScene ? m_pScene->TexImageSet.size() : 0;
    if (NumTexture > CPipelineDepthTest::MaxTextureNum)
    {
        Common::Log::log(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(CPipelineDepthTest::MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = CPipelineDepthTest::MaxTextureNum;
    }
    return NumTexture;
}

void CSceneGoldSrcRenderPass::__calculateVisiableObjects()
{
    if (!m_pScene) return;

    SFrustum Frustum = m_pCamera->getFrustum();

    bool UsePVS = (m_EnableBSP || m_EnableCulling) && m_EnablePVS;

    if (UsePVS)
        m_CameraNodeIndex = m_pScene->BspTree.getPointLeaf(m_pCamera->getPos());
    else
        m_CameraNodeIndex = std::nullopt;

    // calculate PVS
    std::vector<bool> PVS;
    if (UsePVS)
    {
        PVS.resize(m_pScene->Objects.size(), true);
        for (size_t i = 0; i < m_pScene->BspTree.LeafNum; ++i)
        {
            if (!m_pScene->BspPvs.isLeafVisiable(m_CameraNodeIndex.value(), static_cast<uint32_t>(i)))
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

        if (m_pScene->Objects[i]->getMark() == "sky" || m_pScene->Objects[i]->getVertexArray()->empty())
            continue;
        
        if (m_EnableCulling)
        {
            if (m_EnableBSP && i >= m_pScene->BspTree.NodeNum + m_pScene->BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                continue;
            }

            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!__isObjectInSight(m_pScene->Objects[i], Frustum))
                    continue;

            // PVS culling
            if (UsePVS)
                if (!PVS[i])
                    continue;
            
        }

        m_AreObjectsVisable[i] = true;
    }
}

bool CSceneGoldSrcRenderPass::__isObjectInSight(ptr<C3DObject> vpObject, const SFrustum& vFrustum) const
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

void CSceneGoldSrcRenderPass::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 EyeDirection = m_pCamera->getFront();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_PipelineSet.DepthTest.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    m_PipelineSet.BlendTextureAlpha.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    m_PipelineSet.BlendAlphaTest.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    m_PipelineSet.BlendAdditive.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
    m_PipelineSet.Sprite.updateUniformBuffer(vImageIndex, View, Proj, EyePos, EyeDirection);
    if (m_EnableSky)
        m_PipelineSet.Sky.updateUniformBuffer(vImageIndex, View, Proj, EyePos, Up);
}

void CSceneGoldSrcRenderPass::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.Sky.recordCommand(CommandBuffer, vImageIndex);
}
void CSceneGoldSrcRenderPass::__recordSpriteRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.Sprite.recordCommand(CommandBuffer, vImageIndex);
}