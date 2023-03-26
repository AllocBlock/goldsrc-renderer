#include "PassGoldSrc.h"
#include "Common.h"
#include "Function.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"
#include "Log.h"

#include <vector>
#include <set>
#include <fstream>

void CSceneGoldSrcRenderPass::_loadSceneV(ptr<SSceneInfoGoldSrc> vScene)
{
    CRenderPassSceneTyped::_loadSceneV(vScene);

    m_pDevice->waitUntilIdle();
    if (m_pSceneInfo->BspTree.Nodes.empty()) m_EnableBSP = false;
    if (m_pSceneInfo->BspPvs.LeafNum == 0) m_EnablePVS = false;

    m_AreObjectsVisable.clear();
    m_AreObjectsVisable.resize(m_pSceneInfo->pScene->getActorNum(), false);

    m_CurTextureIndex = 0;

    __destroySceneResources();
    __createSceneResources();
    __updateTextureView();
}

void CSceneGoldSrcRenderPass::rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}

void CSceneGoldSrcRenderPass::_initV()
{
    CRenderPassSceneTyped::_initV();
    
    VkExtent2D RefExtent = { 0, 0 };
    _dumpReferenceExtentV(RefExtent);
    m_DepthImageManager.init(RefExtent, false, 
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            vImageSet.init(1);
            VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
            Function::createDepthImage(*vImageSet[0], m_pDevice, vExtent, NULL, DepthFormat);
            m_pPortSet->setOutput("Depth", *vImageSet[0]);
        }
    );

    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();

    auto CreateGoldSrcPipelineFunc = [this](VkExtent2D vExtent, CPipelineGoldSrc& vPipeline)
    {
        if (isValid())
        {
            vPipeline.create(m_pDevice, get(), vExtent);

            VkImageView Lightmap = (m_pSceneInfo && m_pSceneInfo->UseLightmap) ? m_LightmapImage : VK_NULL_HANDLE;
            __updatePipelineResourceGoldSrc(vPipeline);
            rerecordCommand();
        }
    };

    m_PipelineSet.Normal.init(ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendTextureAlpha.init(ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendAlphaTest.init(ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendAdditive.init(ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.Sky.init(ScreenExtent, true, m_pAppInfo->getImageNum(), 
        [this](VkExtent2D vExtent, CPipelineSkybox& vPipeline)
        {
            if (isValid())
            {
                vPipeline.create(m_pDevice, get(), vExtent);
                __updatePipelineResourceSky(vPipeline);
                rerecordCommand();
            }
        });
    m_PipelineSet.Sprite.init(ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](VkExtent2D vExtent, CPipelineSprite& vPipeline)
        {
            if (isValid())
            {
                vPipeline.create(m_pDevice, get(), vExtent);
                __updatePipelineResourceSprite(vPipeline);
                rerecordCommand();
            }
        });

    __createSceneResources();

    rerecordCommand();
}

void CSceneGoldSrcRenderPass::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));

    VkFormat DepthFormat = m_pDevice->getPhysicalDevice()->getBestDepthFormat();
    vioDesc.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::WRITE });
}

CRenderPassDescriptor CSceneGoldSrcRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getOutputPort("Depth"));
}

void CSceneGoldSrcRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    CRenderPassSingle::_onUpdateV(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (!_dumpReferenceExtentV(RefExtent)) return;

    if (m_pCamera)
        m_pCamera->setAspect(RefExtent.width, RefExtent.height);

    m_DepthImageManager.updateV(vUpdateState);
    m_DepthImageManager.updateExtent(RefExtent);

    m_PipelineSet.Sky.updateV(vUpdateState);
    m_PipelineSet.Normal.updateV(vUpdateState);
    m_PipelineSet.BlendTextureAlpha.updateV(vUpdateState);
    m_PipelineSet.BlendAlphaTest.updateV(vUpdateState);
    m_PipelineSet.BlendAdditive.updateV(vUpdateState);
    m_PipelineSet.Sprite.updateV(vUpdateState);
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

    UI::beginWindow(u8"纹理");

    if (!m_TextureImageSet.empty())
    {
        UI::combo(u8"选择纹理", m_TextureComboNameSet, m_CurTextureIndex);
        UI::slider(u8"缩放级别", m_TextureScale, 0.5f, 5.0f, "%.1f");
        const vk::CImage& Image = *m_TextureImageSet[m_CurTextureIndex];
        UI::image(Image, glm::vec2(Image.getWidth() * m_TextureScale, Image.getHeight() * m_TextureScale));
    }
    else
        UI::text(u8"暂无任何纹理");
    UI::endWindow();
    
}

void CSceneGoldSrcRenderPass::_destroyV()
{
    m_DepthImageManager.destroy();
    m_PipelineSet.destroy();

    __destroySceneResources();

    CRenderPassSceneTyped::_destroyV();
}

std::vector<VkCommandBuffer> CSceneGoldSrcRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(isValid());

    m_RenderedObjectSet.clear();
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    bool RerecordCommand = false;
    if (m_EnableBSP || m_EnableCulling || m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        if (m_RerecordCommandTimes > 0) --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        // init
        std::vector<VkClearValue> ClearValueSet(2);
        ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValueSet[1].depthStencil = { 1.0f, 0 };

        _beginWithFramebuffer(vImageIndex);

        if (m_EnableSky)
            __recordSkyRenderCommand(vImageIndex);

        bool Valid = true;
        VkDeviceSize Offsets[] = { 0 };
        if (isNonEmptyAndValid(m_pVertexBuffer))
        {
            VkBuffer VertBuffer = *m_pVertexBuffer;
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        }
        else
            Valid = false;

        if (Valid)
        {
            __calculateVisiableObjects();
            if (m_EnableBSP)
                __renderByBspTree(vImageIndex);
            else
            {
                m_PipelineSet.Normal.get().bind(CommandBuffer, vImageIndex);

                m_PipelineSet.Normal.get().setOpacity(CommandBuffer, 1.0f);

                for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
                {
                    auto pActor = m_pSceneInfo->pScene->getActor(i);
                    bool EnableLightmap = pActor->getMesh()->getMeshData().getEnableLightmap();
                    m_PipelineSet.Normal.get().setLightmapState(CommandBuffer, EnableLightmap);
                    if (m_AreObjectsVisable[i])
                    {
                        __recordObjectRenderCommand(vImageIndex, i);
                        m_RenderedObjectSet.insert(i);
                    }
                }
            }
        }

        if (m_pSceneInfo && !m_pSceneInfo->SprSet.empty())
            __recordSpriteRenderCommand(vImageIndex);

        _endWithFramebuffer();
    }
    return { CommandBuffer };
}

void CSceneGoldSrcRenderPass::__createSceneResources()
{
    __createTextureImages();
    __createLightmapImage();

    m_EnableSky = m_EnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;

    if (m_PipelineSet.Normal.isReady())
        __updatePipelineResourceGoldSrc(m_PipelineSet.Normal.get());
    if (m_PipelineSet.BlendAdditive.isReady())
        __updatePipelineResourceGoldSrc(m_PipelineSet.BlendAdditive.get());
    if (m_PipelineSet.BlendAlphaTest.isReady())
        __updatePipelineResourceGoldSrc(m_PipelineSet.BlendAlphaTest.get());
    if (m_PipelineSet.BlendTextureAlpha.isReady())
        __updatePipelineResourceGoldSrc(m_PipelineSet.BlendTextureAlpha.get());
    if (m_PipelineSet.Sky.isReady())
        __updatePipelineResourceSky(m_PipelineSet.Sky.get());
    if (m_PipelineSet.Sprite.isReady())
        __updatePipelineResourceSprite(m_PipelineSet.Sprite.get());

    rerecordCommand();
}

void CSceneGoldSrcRenderPass::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();
    m_LightmapImage.destroy();
}

void CSceneGoldSrcRenderPass::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeSet.clear();
    if (m_pSceneInfo->BspTree.Nodes.empty()) throw "场景不含BSP数据";

    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Normal.get().bind(CommandBuffer, vImageIndex);

    __renderTreeNode(vImageIndex, 0);
    __renderPointEntities(vImageIndex);
    __renderModels(vImageIndex);
    __renderSprites(vImageIndex);
}

void CSceneGoldSrcRenderPass::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    m_PipelineSet.Normal.get().setOpacity(CommandBuffer, 1.0f);

    if (vNodeIndex >= m_pSceneInfo->BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t LeafIndex = static_cast<uint32_t>(vNodeIndex - m_pSceneInfo->BspTree.NodeNum);
        bool isLeafVisable = false;
        for (size_t ObjectIndex : m_pSceneInfo->BspTree.LeafIndexToObjectIndices.at(LeafIndex))
        {
            if (!m_AreObjectsVisable[ObjectIndex]) continue;
            m_RenderedObjectSet.insert(ObjectIndex);
            isLeafVisable = true;

            bool EnableLightmap = m_pSceneInfo->pScene->getActor(ObjectIndex)->getMesh()->getMeshData().getEnableLightmap();
            m_PipelineSet.Normal.get().setLightmapState(CommandBuffer, EnableLightmap);

            __recordObjectRenderCommand(vImageIndex, ObjectIndex);
        }
        if (isLeafVisable)
            m_RenderNodeSet.insert(LeafIndex);
    }
    else
    {
        const SBspTreeNode& Node = m_pSceneInfo->BspTree.Nodes[vNodeIndex];
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
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

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
    for (size_t i = 0; i < m_pSceneInfo->BspTree.ModelNum; ++i)
    {
        const SModelInfo& ModelInfo = m_pSceneInfo->BspTree.ModelInfos[i];
        SAABB BoundingBox = ModelInfo.BoundingBox;
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
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    _ASSERTE(vModelIndex < m_pSceneInfo->BspTree.ModelInfos.size());

    const SModelInfo& ModelInfo = m_pSceneInfo->BspTree.ModelInfos[vModelIndex];
    CPipelineGoldSrc* pPipeline = nullptr;

    switch (ModelInfo.RenderMode)
    {
    case EGoldSrcRenderMode::NORMAL:
    {
        pPipeline = &m_PipelineSet.Normal.get();
        break;
    }
    case EGoldSrcRenderMode::COLOR:
    case EGoldSrcRenderMode::TEXTURE:
    {
        pPipeline = &m_PipelineSet.BlendTextureAlpha.get();
        break;
    }
    case EGoldSrcRenderMode::SOLID:
    {
        pPipeline = &m_PipelineSet.BlendAlphaTest.get();
        break;
    }
    case EGoldSrcRenderMode::ADDITIVE:
    case EGoldSrcRenderMode::GLOW:
    {
        pPipeline = &m_PipelineSet.BlendAdditive.get();
        break;
    }
    default:
        break;
    }

    pPipeline->bind(CommandBuffer, vImageIndex);
    pPipeline->setOpacity(CommandBuffer, ModelInfo.Opacity);

    std::vector<size_t> ObjectIndices = m_pSceneInfo->BspTree.ModelIndexToObjectIndex[vModelIndex];
    for (size_t ObjectIndex : ObjectIndices)
    {
        if (!m_AreObjectsVisable[ObjectIndex]) continue;

        bool EnableLightmap = m_pSceneInfo->pScene->getActor(ObjectIndex)->getMesh()->getMeshData().getEnableLightmap();
        pPipeline->setLightmapState(CommandBuffer, EnableLightmap);
        __recordObjectRenderCommand(vImageIndex, ObjectIndex);
    }
}

void CSceneGoldSrcRenderPass::__renderPointEntities(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Normal.get().bind(CommandBuffer, vImageIndex);

    for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
    {
        if (!m_AreObjectsVisable[i]) continue;
        else if (m_pSceneInfo->pScene->getActor(i)->hasTag("point_entity")) continue;
        __recordObjectRenderCommand(vImageIndex, i);
    }
}

void CSceneGoldSrcRenderPass::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    _recordRenderActorCommand(CommandBuffer, vObjectIndex);
}

void CSceneGoldSrcRenderPass::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.init(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            Function::createImageFromIOImage(*m_TextureImageSet[i], m_pDevice, m_pSceneInfo->TexImageSet[i]);
        }
    }
}

void CSceneGoldSrcRenderPass::__createLightmapImage()
{
    if (m_pSceneInfo && m_pSceneInfo->UseLightmap)
    {
        ptr<CIOImage> pCombinedLightmapImage = m_pSceneInfo->pLightmap->getCombinedLightmap();
        Function::createImageFromIOImage(m_LightmapImage, m_pDevice, pCombinedLightmapImage);
    }
}

void CSceneGoldSrcRenderPass::__updateTextureView()
{
    size_t ImageNum = m_pSceneInfo->TexImageSet.size();
    _ASSERTE(ImageNum == m_TextureImageSet.size());
    m_CurTextureIndex = std::max<int>(0, std::min<int>(int(ImageNum), m_CurTextureIndex));
    m_TextureNameSet.resize(ImageNum);
    m_TextureComboNameSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        m_TextureNameSet[i] = std::to_string(i + 1) + ": " + m_pSceneInfo->TexImageSet[i]->getName();
        m_TextureComboNameSet[i] = m_TextureNameSet[i].c_str();
    }
}

size_t CSceneGoldSrcRenderPass::__getActualTextureNum()
{
    size_t NumTexture = m_pSceneInfo ? m_pSceneInfo->TexImageSet.size() : 0;
    if (NumTexture > CPipelineNormal::MaxTextureNum)
    {
        Log::log("警告: 纹理数量 = (" + std::to_string(NumTexture) + ") 大于限制数量 (" + std::to_string(CPipelineNormal::MaxTextureNum) + "), 多出的纹理将被忽略");
        NumTexture = CPipelineNormal::MaxTextureNum;
    }
    return NumTexture;
}

void CSceneGoldSrcRenderPass::__updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline)
{
    vPipeline.clearResources();
    vPipeline.setTextures(m_TextureImageSet);
    vPipeline.setLightmap(m_LightmapImage);
}

void CSceneGoldSrcRenderPass::__updatePipelineResourceSky(CPipelineSkybox& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (m_pSceneInfo->UseSkyBox && vPipeline.isValid())
        {
            vPipeline.setSkyBoxImage(m_pSceneInfo->SkyBoxImages);
        }
    }
}

void CSceneGoldSrcRenderPass::__updatePipelineResourceSprite(CPipelineSprite& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (!m_pSceneInfo->SprSet.empty() && vPipeline.isValid())
        {
            vPipeline.setSprites(m_pSceneInfo->SprSet);
        }
    }
}

void CSceneGoldSrcRenderPass::__calculateVisiableObjects()
{
    if (!m_pSceneInfo) return;

    SFrustum Frustum = m_pCamera->getFrustum();

    bool UsePVS = (m_EnableBSP || m_EnableCulling) && m_EnablePVS;

    if (UsePVS)
        m_CameraNodeIndex = m_pSceneInfo->BspTree.getPointLeaf(m_pCamera->getPos());
    else
        m_CameraNodeIndex = std::nullopt;

    // calculate PVS
    std::vector<bool> PVS;
    if (UsePVS)
    {
        PVS.resize(m_pSceneInfo->pScene->getActorNum(), true);
        for (size_t i = 0; i < m_pSceneInfo->BspTree.LeafNum; ++i)
        {
            if (!m_pSceneInfo->BspPvs.isLeafVisiable(m_CameraNodeIndex.value(), static_cast<uint32_t>(i)))
            {
                std::vector<size_t> LeafObjectIndices = m_pSceneInfo->BspTree.LeafIndexToObjectIndices[i];
                for (size_t LeafObjectIndex : LeafObjectIndices)
                    PVS[LeafObjectIndex] = false;
            }
        }
    }

    for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
    {
        auto pActor = m_pSceneInfo->pScene->getActor(i);
        m_AreObjectsVisable[i] = false;

        if (pActor->hasTag("sky") || m_pVertexBuffer->getSegmentInfo(i).Num == 0)
            continue;
        
        if (m_EnableCulling)
        {
            if (m_EnableBSP && i >= m_pSceneInfo->BspTree.NodeNum + m_pSceneInfo->BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                continue;
            }

            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!isActorInSight(pActor, Frustum))
                    continue;

            // PVS culling
            if (UsePVS)
                if (!PVS[i])
                    continue;
            
        }

        m_AreObjectsVisable[i] = true;
    }
}

void CSceneGoldSrcRenderPass::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Normal.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendTextureAlpha.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendAlphaTest.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendAdditive.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.Sprite.get().updateUniformBuffer(vImageIndex, m_pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.get().updateUniformBuffer(vImageIndex, m_pCamera);
}

void CSceneGoldSrcRenderPass::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Sky.get().recordCommand(CommandBuffer, vImageIndex);
}
void CSceneGoldSrcRenderPass::__recordSpriteRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Sprite.get().recordCommand(CommandBuffer, vImageIndex);
}