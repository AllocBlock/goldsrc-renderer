#include "PassGoldSrc.h"
#include "Common.h"
#include "ImageUtils.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"
#include "Log.h"
#include "ComponentIconRenderer.h"

#include <vector>
#include <set>
#include <fstream>

void CRenderPassGoldSrc::_onSceneInfoSet(ptr<SSceneInfo> vScene)
{
    m_pDevice->waitUntilIdle();
    m_CurTextureIndex = 0;

    __createVertexBuffer();
    __destroySceneResources();
    __createSceneResources();
    __updateTextureView();
}

void CRenderPassGoldSrc::rerecordAllCommand()
{
    m_pRerecord->requestRecordForAll();
}

void CRenderPassGoldSrc::_initV()
{
    size_t ImageNum = m_pAppInfo->getImageNum();

    m_pRerecord = make<CRerecordState>(ImageNum);
    m_pRerecord->addField("Primary");
    for (const auto& Name : _getExtraCommandBufferNamesV())
        m_pRerecord->addField(Name);
    
    VkExtent2D RefExtent = { 0, 0 };
    bool Success = _dumpReferenceExtentV(RefExtent);
    _ASSERTE(Success);

    VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
    ImageUtils::createDepthImage(m_DepthImage, m_pDevice, RefExtent, NULL, DepthFormat);
    m_pPortSet->setOutput("Depth", m_DepthImage);

    CRenderPassSingleFrameBuffer::_initV();

    m_PipelineSet.Normal.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.BlendTextureAlpha.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.BlendAlphaTest.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.BlendAdditive.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.Simple.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.Sky.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.Sprite.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.Icon.create(m_pDevice, get(), RefExtent, ImageNum);
    m_PipelineSet.Text.create(m_pDevice, get(), RefExtent, ImageNum);
    
    __createSceneResources();

    rerecordAllCommand();
}

void CRenderPassGoldSrc::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));

    VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;
    vioDesc.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::WRITE });
}

CRenderPassDescriptor CRenderPassGoldSrc::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
                                                            m_pPortSet->getOutputPort("Depth"));
}

void CRenderPassGoldSrc::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CRenderPassGoldSrc::_renderUIV()
{
    if (UI::collapse(u8"渲染设置"))
    {
        static const std::vector<const char*> RenderMethodNames =
        {
            u8"金源渲染",
            u8"简易",
        };
        
        int RenderMethodIndex = static_cast<int>(m_RenderMethod);
        if (UI::combo(u8"渲染器", RenderMethodNames, RenderMethodIndex))
        {
            setRenderMethod(static_cast<ERenderMethod>(RenderMethodIndex));
        }

        bool SkyRendering = getSkyState();
        UI::toggle(u8"开启天空渲染", SkyRendering);
        setSkyState(SkyRendering);
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

void CRenderPassGoldSrc::_destroyV()
{
    m_DepthImage.destroy();
    m_PipelineSet.destroy();
    destroyAndClear(m_pVertexBuffer);

    __destroySceneResources();

    CRenderPassSingleFrameBuffer::_destroyV();
}

std::vector<VkCommandBuffer> CRenderPassGoldSrc::_requestCommandBuffersV(uint32_t vImageIndex)
{
    _ASSERTE(isValid());

    bool NeedRecordPrimary = false;
    
    // sky
    CCommandBuffer::Ptr pSkyCmdBuffer = m_Command.getCommandBuffer("Sky", vImageIndex);
    if (m_pRerecord->consume("Sky"))
    {
        _beginSecondary(pSkyCmdBuffer, vImageIndex);
        if (m_EnableSky)
        {
            m_PipelineSet.Sky.recordCommand(pSkyCmdBuffer, vImageIndex);
        }
        pSkyCmdBuffer->end();
        NeedRecordPrimary = true;
    }

    // mesh
    CCommandBuffer::Ptr pMeshCmdBuffer = m_Command.getCommandBuffer("Mesh", vImageIndex);
    if (m_pRerecord->consume("Mesh"))
    {
        _beginSecondary(pMeshCmdBuffer, vImageIndex);
        if (isNonEmptyAndValid(m_pVertexBuffer))
        {
            pMeshCmdBuffer->bindVertexBuffer(*m_pVertexBuffer);
    
            if (m_RenderMethod == ERenderMethod::GOLDSRC)
            {
                m_PipelineSet.Normal.bind(pMeshCmdBuffer, vImageIndex);
                m_PipelineSet.Normal.setOpacity(pMeshCmdBuffer, 1.0f);
            }
            else if (m_RenderMethod == ERenderMethod::SIMPLE)
            {
                m_PipelineSet.Simple.bind(pMeshCmdBuffer, vImageIndex);
            }
            else
            {
                _SHOULD_NOT_GO_HERE;
            }

            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);
                if (!pActor->getVisible()) continue;
                if (m_ActorSegmentMap.find(pActor) == m_ActorSegmentMap.end()) continue;

                auto pTransform = pActor->getTransform();
                auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
                if (!pMeshRenderer) continue;

                auto pMesh = pMeshRenderer->getMesh();
                if (!pMesh) continue;

                if (m_RenderMethod == ERenderMethod::GOLDSRC)
                {
                    bool EnableLightmap = pMesh->getMeshDataV().hasLightmap();
                    m_PipelineSet.Normal.setLightmapState(pMeshCmdBuffer, EnableLightmap);
                }
                __drawMeshActor(pMeshCmdBuffer, pActor);
            }
        }
        pMeshCmdBuffer->end();
        NeedRecordPrimary = true;
    }

    // sprite
    CCommandBuffer::Ptr pSpriteCmdBuffer = m_Command.getCommandBuffer("Sprite", vImageIndex);
    if (m_pRerecord->consume("Sprite"))
    {
        _beginSecondary(pSpriteCmdBuffer, vImageIndex);
        if (m_pSceneInfo && !m_pSceneInfo->SprSet.empty())
        {
            m_PipelineSet.Sprite.recordCommand(pSpriteCmdBuffer, vImageIndex);
        }
        pSpriteCmdBuffer->end();
        NeedRecordPrimary = true;
    }

    // icon
    CCommandBuffer::Ptr pIconCmdBuffer = m_Command.getCommandBuffer("Icon", vImageIndex);
    if (m_pRerecord->consume("Icon"))
    {
        _beginSecondary(pIconCmdBuffer, vImageIndex);
        if (m_pSceneInfo)
        {
            auto& PipelineIcon = m_PipelineSet.Icon;
            PipelineIcon.clear();
            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);
                if (!pActor->getVisible()) continue;

                auto pIconRenderer = pActor->getTransform()->findComponent<CComponentIconRenderer>();
                if (!pIconRenderer || !pIconRenderer->hasIcon()) continue;

                EIcon Icon = pIconRenderer->getIcon();
                glm::vec3 Position = pActor->getTransform()->getAbsoluteTranslate();
                glm::vec3 Scale = pActor->getTransform()->getAbsoluteScale();
                PipelineIcon.addIcon(Icon, Position, glm::max(Scale.x, glm::max(Scale.y, Scale.z)));
            }
            PipelineIcon.recordCommand(pIconCmdBuffer, vImageIndex);
        }
        pIconCmdBuffer->end();
        NeedRecordPrimary = true;
    }

    // text
    CCommandBuffer::Ptr pTextCmdBuffer = m_Command.getCommandBuffer("Text", vImageIndex);
    auto& PipelineText = m_PipelineSet.Text;
    if (PipelineText.doesNeedRerecord(vImageIndex))
    {
        _beginSecondary(pTextCmdBuffer, vImageIndex);
        PipelineText.recordCommand(pTextCmdBuffer, vImageIndex);
        pTextCmdBuffer->end();
        NeedRecordPrimary = true;
    }

    // primary
    CCommandBuffer::Ptr pPrimaryCmdBuffer = _getCommandBuffer(vImageIndex);
    if (m_pRerecord->consume("Primary") || NeedRecordPrimary)
    {
        _beginWithFramebuffer(vImageIndex, true);
        pPrimaryCmdBuffer->execCommand(pSkyCmdBuffer->get());
        pPrimaryCmdBuffer->execCommand(pMeshCmdBuffer->get());
        pPrimaryCmdBuffer->execCommand(pSpriteCmdBuffer->get());
        pPrimaryCmdBuffer->execCommand(pIconCmdBuffer->get());
        pPrimaryCmdBuffer->execCommand(pTextCmdBuffer->get());
        _endWithFramebuffer();
    }
    
    return { pPrimaryCmdBuffer->get() };
}

void CRenderPassGoldSrc::__createSceneResources()
{
    __createTextureImages();
    __createLightmapImage();

    m_EnableSky = m_EnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;

    __updatePipelineResourceGoldSrc(m_PipelineSet.Normal);
    __updatePipelineResourceGoldSrc(m_PipelineSet.BlendAdditive);
    __updatePipelineResourceGoldSrc(m_PipelineSet.BlendAlphaTest);
    __updatePipelineResourceGoldSrc(m_PipelineSet.BlendTextureAlpha);
    __updatePipelineResourceSimple(m_PipelineSet.Simple);
    __updatePipelineResourceSky(m_PipelineSet.Sky);
    __updatePipelineResourceSprite(m_PipelineSet.Sprite);

    // search all text renderer
    if (m_pSceneInfo)
    {
        auto& PipelineText = m_PipelineSet.Text;
        PipelineText.clearTextComponent();
        for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
        {
            auto pActor = m_pSceneInfo->pScene->getActor(i);
            if (!pActor->getVisible()) continue;

            auto pTextRenderer = pActor->getTransform()->findComponent<CComponentTextRenderer>();
            if (!pTextRenderer) continue;

            PipelineText.addTextComponent(pTextRenderer);
        }
    }

    m_pRerecord->requestRecordForAll();
}

void CRenderPassGoldSrc::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();
    m_LightmapImage.destroy();
}

//struct SModelSortInfo
//{
//    size_t Index;
//    EGoldSrcRenderMode RenderMode;
//    float Distance;
//
//    int getRenderModePriority() const
//    {
//        switch (RenderMode)
//        {
//        case EGoldSrcRenderMode::NORMAL:
//        case EGoldSrcRenderMode::SOLID:
//            return 0;
//        case EGoldSrcRenderMode::COLOR:
//        case EGoldSrcRenderMode::TEXTURE:
//            return 1;
//        case EGoldSrcRenderMode::GLOW:
//        case EGoldSrcRenderMode::ADDITIVE:
//            return 2;
//        default: return -1;
//        }
//    }
//};

// FIXME: restore this part later
//void CSceneGoldSrcRenderPass::__renderModel(uint32_t vImageIndex, size_t vModelIndex)
//{
//    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
//
//    _ASSERTE(vModelIndex < m_pSceneInfo->BspTree.ModelInfos.size());
//
//    const SModelInfo& ModelInfo = m_pSceneInfo->BspTree.ModelInfos[vModelIndex];
//    CPipelineGoldSrc* pPipeline = nullptr;
//
//    switch (ModelInfo.RenderMode)
//    {
//    case EGoldSrcRenderMode::NORMAL:
//    {
//        pPipeline = &m_PipelineSet.Normal;
//        break;
//    }
//    case EGoldSrcRenderMode::COLOR:
//    case EGoldSrcRenderMode::TEXTURE:
//    {
//        pPipeline = &m_PipelineSet.BlendTextureAlpha;
//        break;
//    }
//    case EGoldSrcRenderMode::SOLID:
//    {
//        pPipeline = &m_PipelineSet.BlendAlphaTest;
//        break;
//    }
//    case EGoldSrcRenderMode::ADDITIVE:
//    case EGoldSrcRenderMode::GLOW:
//    {
//        pPipeline = &m_PipelineSet.BlendAdditive;
//        break;
//    }
//    default:
//        break;
//    }
//
//    pPipeline->bind(CommandBuffer, vImageIndex);
//    pPipeline->setOpacity(CommandBuffer, ModelInfo.Opacity);
//
//    std::vector<size_t> ObjectIndices = m_pSceneInfo->BspTree.ModelIndexToObjectIndex[vModelIndex];
//    for (size_t ObjectIndex : ObjectIndices)
//    {
//        if (!m_AreObjectsVisable[ObjectIndex]) continue;
//        __renderActorByPipeline(vImageIndex, ObjectIndex, CommandBuffer, *pPipeline);
//    }
//}

void CRenderPassGoldSrc::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.init(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            ImageUtils::createImageFromIOImage(*m_TextureImageSet[i], m_pDevice, m_pSceneInfo->TexImageSet[i]);
        }
    }
}

void CRenderPassGoldSrc::__createLightmapImage()
{
    if (m_pSceneInfo && m_pSceneInfo->UseLightmap)
    {
        ptr<CIOImage> pCombinedLightmapImage = m_pSceneInfo->pLightmap->getCombinedLightmap();
        ImageUtils::createImageFromIOImage(m_LightmapImage, m_pDevice, pCombinedLightmapImage);
    }
}

void CRenderPassGoldSrc::__createVertexBuffer()
{
    destroyAndClear(m_pVertexBuffer);
    if (m_RenderMethod == ERenderMethod::GOLDSRC)
    {
        const auto& Pair = m_pSceneInfo->pScene->generateVertexBuffer<SGoldSrcPointData>(m_pDevice);

        m_pVertexBuffer = Pair.first;
        m_ActorSegmentMap = Pair.second;
    }
    else if (m_RenderMethod == ERenderMethod::SIMPLE)
    {
        const auto& Pair = m_pSceneInfo->pScene->generateVertexBuffer<SSimplePointData>(m_pDevice);

        m_pVertexBuffer = Pair.first;
        m_ActorSegmentMap = Pair.second;
    }
    else
    {
        _SHOULD_NOT_GO_HERE;
    }
}

void CRenderPassGoldSrc::__updateTextureView()
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

size_t CRenderPassGoldSrc::__getActualTextureNum()
{
    size_t NumTexture = m_pSceneInfo ? m_pSceneInfo->TexImageSet.size() : 0;
    if (NumTexture > CPipelineNormal::MaxTextureNum)
    {
        Log::log("警告: 纹理数量 = (" + std::to_string(NumTexture) + ") 大于限制数量 (" + std::to_string(CPipelineNormal::MaxTextureNum) + "), 多出的纹理将被忽略");
        NumTexture = CPipelineNormal::MaxTextureNum;
    }
    return NumTexture;
}

void CRenderPassGoldSrc::__updatePipelineResourceGoldSrc(CPipelineGoldSrc& vPipeline)
{
    vPipeline.clearResources();
    vPipeline.setTextures(m_TextureImageSet);
    vPipeline.setLightmap(m_LightmapImage);
}

void CRenderPassGoldSrc::__updatePipelineResourceSimple(CPipelineSimple& vPipeline)
{
    vPipeline.setTextures(m_TextureImageSet);
}

void CRenderPassGoldSrc::__updatePipelineResourceSky(CPipelineSkybox& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (m_pSceneInfo->UseSkyBox && vPipeline.isValid())
        {
            vPipeline.setSkyBoxImage(m_pSceneInfo->SkyBoxImages);
        }
    }
}

void CRenderPassGoldSrc::__updatePipelineResourceSprite(CPipelineSprite& vPipeline)
{
    if (m_pSceneInfo)
    {
        if (!m_pSceneInfo->SprSet.empty() && vPipeline.isValid())
        {
            vPipeline.setSprites(m_pSceneInfo->SprSet);
        }
    }
}

void CRenderPassGoldSrc::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    auto pCamera = m_pSceneInfo->pScene->getMainCamera();

    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Normal.updateUniformBuffer(vImageIndex, Model, pCamera);
    m_PipelineSet.BlendTextureAlpha.updateUniformBuffer(vImageIndex, Model, pCamera);
    m_PipelineSet.BlendAlphaTest.updateUniformBuffer(vImageIndex, Model, pCamera);
    m_PipelineSet.BlendAdditive.updateUniformBuffer(vImageIndex, Model, pCamera);
    m_PipelineSet.Simple.updateUniformBuffer(vImageIndex, Model, pCamera);
    m_PipelineSet.Sprite.updateUniformBuffer(vImageIndex, pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.updateUniformBuffer(vImageIndex, pCamera);
    m_PipelineSet.Icon.updateUniformBuffer(vImageIndex, pCamera);
    m_PipelineSet.Text.updateUniformBuffer(vImageIndex, pCamera);
}
