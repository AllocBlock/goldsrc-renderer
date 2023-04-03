#include "PassGoldSrcDeprecated.h"
#include "Common.h"
#include "Function.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"
#include "Log.h"

#include <vector>
#include <set>
#include <fstream>

void CSceneGoldSrcRenderPassDeprecated::_loadSceneV(ptr<SSceneInfoGoldSrc> vScene)
{
    CRenderPassSceneTyped::_loadSceneV(vScene);

    if (m_pSceneInfo->BspTree.Nodes.empty()) m_EnableBSP = false;
    if (m_pSceneInfo->BspPvs.LeafNum == 0) m_EnablePVS = false;

    m_AreObjectsVisable.clear();
    m_AreObjectsVisable.resize(m_pSceneInfo->pScene->getActorNum(), false);

    m_CurTextureIndex = 0;

    __destroySceneResources();
    __createSceneResources();
    __updateTextureView();
}

void CSceneGoldSrcRenderPassDeprecated::rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}

void CSceneGoldSrcRenderPassDeprecated::_initV()
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

    auto CreateGoldSrcPipelineFunc = [this](CPipelineGoldSrc& vPipeline)
    {
        if (vPipeline.isValid())
        {
            VkImageView Lightmap = (m_pSceneInfo && m_pSceneInfo->UseLightmap) ? m_LightmapImage : VK_NULL_HANDLE;
            __updatePipelineResourceGoldSrc(vPipeline);
            rerecordCommand();
        }
    };

    m_PipelineSet.Normal.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendTextureAlpha.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendAlphaTest.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendAdditive.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.Sky.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](CPipelineSkybox& vPipeline)
        {
            if (vPipeline.isValid())
            {
                __updatePipelineResourceSky(vPipeline);
                rerecordCommand();
            }
        });
    m_PipelineSet.Sprite.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](CPipelineSprite& vPipeline)
        {
            if (vPipeline.isValid())
            {
                __updatePipelineResourceSprite(vPipeline);
                rerecordCommand();
            }
        });
    m_PipelineSet.Icon.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](IPipeline& vPipeline)
        {
            rerecordCommand();
        });

    __createSceneResources();

    rerecordCommand();
}

void CSceneGoldSrcRenderPassDeprecated::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));

    VkFormat DepthFormat = m_pDevice->getPhysicalDevice()->getBestDepthFormat();
    vioDesc.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::WRITE });
}

CRenderPassDescriptor CSceneGoldSrcRenderPassDeprecated::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getOutputPort("Depth"));
}

void CSceneGoldSrcRenderPassDeprecated::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_DepthImageManager.updateV(vUpdateState);
    m_PipelineSet.update(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (_dumpReferenceExtentV(RefExtent))
    {
        if (m_pCamera)
            m_pCamera->setAspect(RefExtent.width, RefExtent.height);

        m_DepthImageManager.updateExtent(RefExtent);
    }

    CRenderPassSingle::_onUpdateV(vUpdateState);
}

void CSceneGoldSrcRenderPassDeprecated::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CSceneGoldSrcRenderPassDeprecated::_renderUIV()
{
    if (UI::collapse(u8"�½�Դ"))
    {
        bool EnableBSP = getBSPState();
        UI::toggle(u8"ʹ��BSP����Ⱦ��֧��ʵ����Ⱦģʽ��", EnableBSP);
        setBSPState(EnableBSP);

        bool SkyRendering = getSkyState();
        UI::toggle(u8"���������Ⱦ", SkyRendering);
        setSkyState(SkyRendering);

        bool Culling = getCullingState();
        UI::toggle(u8"�����޳�", Culling);
        setCullingState(Culling);
        Culling = getCullingState();

        if (Culling)
        {
            UI::indent(20.0f);
            bool FrustumCulling = getFrustumCullingState();
            UI::toggle(u8"CPU��׶�޳�", FrustumCulling);
            setFrustumCullingState(FrustumCulling);

            bool PVS = getPVSState();
            UI::toggle(u8"PVS�޳�", PVS);
            setPVSState(PVS);
            UI::unindent();
        }

        std::optional<uint32_t> CameraNodeIndex = getCameraNodeIndex();
        if (CameraNodeIndex == std::nullopt)
            UI::text(u8"��������ڵ㣺-");
        else
            UI::text((u8"��������ڵ㣺" + std::to_string(CameraNodeIndex.value())).c_str());

        if (!getBSPState())
        {
            std::set<size_t> RenderObjectList = getRenderedObjectSet();
            UI::text((u8"��Ⱦ��������" + std::to_string(RenderObjectList.size())).c_str());
            if (!RenderObjectList.empty())
            {
                std::string RenderNodeListStr = "";
                for (size_t ObjectIndex : RenderObjectList)
                {
                    RenderNodeListStr += std::to_string(ObjectIndex) + ", ";
                }
                UI::text((u8"��Ⱦ���壺" + RenderNodeListStr).c_str(), true);
            }
        }
        else
        {
            std::set<uint32_t> RenderNodeList = getRenderedNodeList();
            UI::text((u8"��Ⱦ�ڵ�����" + std::to_string(RenderNodeList.size())).c_str());
            if (!RenderNodeList.empty())
            {
                std::string RenderNodeListStr = "";
                for (uint32_t NodeIndex : RenderNodeList)
                {
                    RenderNodeListStr += std::to_string(NodeIndex) + ", ";
                }
                UI::text((u8"��Ⱦ�ڵ㣺" + RenderNodeListStr).c_str(), true);
            }
        }
    }

    UI::beginWindow(u8"����");

    if (!m_TextureImageSet.empty())
    {
        UI::combo(u8"ѡ������", m_TextureComboNameSet, m_CurTextureIndex);
        UI::slider(u8"���ż���", m_TextureScale, 0.5f, 5.0f, "%.1f");
        const vk::CImage& Image = *m_TextureImageSet[m_CurTextureIndex];
        UI::image(Image, glm::vec2(Image.getWidth() * m_TextureScale, Image.getHeight() * m_TextureScale));
    }
    else
        UI::text(u8"�����κ�����");
    UI::endWindow();
    
}

void CSceneGoldSrcRenderPassDeprecated::_destroyV()
{
    m_DepthImageManager.destroy();
    m_PipelineSet.destroy();

    __destroySceneResources();

    CRenderPassSceneTyped::_destroyV();
}

std::vector<VkCommandBuffer> CSceneGoldSrcRenderPassDeprecated::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(isValid());

    m_RenderedObjectSet.clear();
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

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
        if (isNonEmptyAndValid(m_pVertexBuffer))
        {
            pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
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
                m_PipelineSet.Normal.get().bind(pCommandBuffer, vImageIndex);

                m_PipelineSet.Normal.get().setOpacity(pCommandBuffer, 1.0f);

                for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
                {
                    if (m_AreObjectsVisable[i])
                    {
                        __renderActorByPipeline(vImageIndex, i, pCommandBuffer, m_PipelineSet.Normal.get());
                        m_RenderedObjectSet.insert(i);
                    }
                }
            }
        }

        if (m_pSceneInfo && !m_pSceneInfo->SprSet.empty())
            __recordSpriteRenderCommand(vImageIndex);

        // render icon
        if (m_pSceneInfo)
        {
            auto& PipelineIcon = m_PipelineSet.Icon.get();
            PipelineIcon.clear();
            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);

                auto pTransform = pActor->getTransform();
                auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
                if (pMeshRenderer) continue;
                // TODO: change to find if there is icon renderer!

                glm::vec3 Position = pActor->getTransform()->getAbsoluteTranslate();
                PipelineIcon.addIcon(EIcon::TIP, Position);
            }
            PipelineIcon.recordCommand(pCommandBuffer, vImageIndex);
        }

        _endWithFramebuffer();
    }
    return { pCommandBuffer->get() };
}

void CSceneGoldSrcRenderPassDeprecated::__createSceneResources()
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

void CSceneGoldSrcRenderPassDeprecated::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();
    m_LightmapImage.destroy();
}

void CSceneGoldSrcRenderPassDeprecated::__renderByBspTree(uint32_t vImageIndex)
{
    m_RenderNodeSet.clear();
    if (m_pSceneInfo->BspTree.Nodes.empty()) throw "��������BSP����";

    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Normal.get().bind(pCommandBuffer, vImageIndex);

    __renderTreeNode(vImageIndex, 0);
    //__renderPointEntities(vImageIndex);
    __renderModels(vImageIndex);
    __renderSprites(vImageIndex);
}

void CSceneGoldSrcRenderPassDeprecated::__renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    m_PipelineSet.Normal.get().setOpacity(pCommandBuffer, 1.0f);

    if (vNodeIndex >= m_pSceneInfo->BspTree.NodeNum) // if is leaf, render it
    {
        uint32_t LeafIndex = static_cast<uint32_t>(vNodeIndex - m_pSceneInfo->BspTree.NodeNum);
        bool isLeafVisable = false;
        for (size_t ObjectIndex : m_pSceneInfo->BspTree.LeafIndexToObjectIndices.at(LeafIndex))
        {
            if (!m_AreObjectsVisable[ObjectIndex]) continue;
            m_RenderedObjectSet.insert(ObjectIndex);
            isLeafVisable = true;

            __renderActorByPipeline(vImageIndex, ObjectIndex, pCommandBuffer, m_PipelineSet.Normal.get());
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

void CSceneGoldSrcRenderPassDeprecated::__renderModels(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

    // ����ȥ������Xash3D��Դ�룬xash3d/engine/client/gl_rmain.c
    // ���ǰ������������Ӻͷ����˳�����
    // 
    // Ҳ��ʵ����м򵥵İ���������
    auto SortedModelSet = __sortModelRenderSequence();

    for (size_t ModelIndex : SortedModelSet)
    {
        __renderModel(vImageIndex, ModelIndex);
    }
}

void CSceneGoldSrcRenderPassDeprecated::__renderSprites(uint32_t vImageIndex)
{

}

void CSceneGoldSrcRenderPassDeprecated::__renderActorByPipeline(uint32_t vImageIndex, size_t vActorIndex, CCommandBuffer::Ptr vCommandBuffer, CPipelineGoldSrc& vPipeline)
{
    auto pActor = m_pSceneInfo->pScene->getActor(vActorIndex);

    auto pTransform = pActor->getTransform();
    auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
    if (!pMeshRenderer) return;

    auto pMesh = pMeshRenderer->getMesh();
    if (!pMesh) return;

    bool EnableLightmap = pMesh->getMeshDataV().hasLightmap();
    vPipeline.setLightmapState(vCommandBuffer, EnableLightmap);
    __drawActor(vImageIndex, pActor);
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

std::vector<size_t> CSceneGoldSrcRenderPassDeprecated::__sortModelRenderSequence()
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

void CSceneGoldSrcRenderPassDeprecated::__renderModel(uint32_t vImageIndex, size_t vModelIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

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

    pPipeline->bind(pCommandBuffer, vImageIndex);
    pPipeline->setOpacity(pCommandBuffer, ModelInfo.Opacity);

    std::vector<size_t> ObjectIndices = m_pSceneInfo->BspTree.ModelIndexToObjectIndex[vModelIndex];
    for (size_t ObjectIndex : ObjectIndices)
    {
        if (!m_AreObjectsVisable[ObjectIndex]) continue;
        __renderActorByPipeline(vImageIndex, ObjectIndex, pCommandBuffer, *pPipeline);
    }
}

void CSceneGoldSrcRenderPassDeprecated::__renderPointEntities(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Normal.get().bind(pCommandBuffer, vImageIndex);

    for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
    {
        if (!m_AreObjectsVisable[i]) continue;
        else if (m_pSceneInfo->pScene->getActor(i)->hasTag("point_entity")) continue;
        __drawActor(vImageIndex, m_pSceneInfo->pScene->getActor(i));
    }
}

void CSceneGoldSrcRenderPassDeprecated::__drawActor(uint32_t vImageIndex, CActor::Ptr vActor)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    _drawActor(pCommandBuffer, vActor);
}

void CSceneGoldSrcRenderPassDeprecated::__createTextureImages()
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

void CSceneGoldSrcRenderPassDeprecated::__createLightmapImage()
{
    if (m_pSceneInfo && m_pSceneInfo->UseLightmap)
    {
        ptr<CIOImage> pCombinedLightmapImage = m_pSceneInfo->pLightmap->getCombinedLightmap();
        Function::createImageFromIOImage(m_LightmapImage, m_pDevice, pCombinedLightmapImage);
    }
}

void CSceneGoldSrcRenderPassDeprecated::__updateTextureView()
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

size_t CSceneGoldSrcRenderPassDeprecated::__getActualTextureNum()
{
    size_t NumTexture = m_pSceneInfo ? m_pSceneInfo->TexImageSet.size() : 0;
    if (NumTexture > CPipelineNormal::MaxTextureNum)
    {
        Log::log("����: �������� = (" + std::to_string(NumTexture) + ") ������������ (" + std::to_string(CPipelineNormal::MaxTextureNum) + "), �����������������");
        NumTexture = CPipelineNormal::MaxTextureNum;
    }
    return NumTexture;
}

void CSceneGoldSrcRenderPassDeprecated::__updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline)
{
    vPipeline.clearResources();
    vPipeline.setTextures(m_TextureImageSet);
    vPipeline.setLightmap(m_LightmapImage);
}

void CSceneGoldSrcRenderPassDeprecated::__updatePipelineResourceSky(CPipelineSkybox& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (m_pSceneInfo->UseSkyBox && vPipeline.isValid())
        {
            vPipeline.setSkyBoxImage(m_pSceneInfo->SkyBoxImages);
        }
    }
}

void CSceneGoldSrcRenderPassDeprecated::__updatePipelineResourceSprite(CPipelineSprite& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (!m_pSceneInfo->SprSet.empty() && vPipeline.isValid())
        {
            vPipeline.setSprites(m_pSceneInfo->SprSet);
        }
    }
}

void CSceneGoldSrcRenderPassDeprecated::__calculateVisiableObjects()
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

        if (pActor->hasTag("sky") || m_ActorSegmentMap.find(pActor) == m_ActorSegmentMap.end())
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

void CSceneGoldSrcRenderPassDeprecated::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Normal.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendTextureAlpha.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendAlphaTest.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendAdditive.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.Sprite.get().updateUniformBuffer(vImageIndex, m_pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.get().updateUniformBuffer(vImageIndex, m_pCamera);
    m_PipelineSet.Icon.get().updateUniformBuffer(vImageIndex, m_pCamera);
}

void CSceneGoldSrcRenderPassDeprecated::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Sky.get().recordCommand(pCommandBuffer, vImageIndex);
}
void CSceneGoldSrcRenderPassDeprecated::__recordSpriteRenderCommand(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Sprite.get().recordCommand(pCommandBuffer, vImageIndex);
}