#include "PassGoldSrc.h"
#include "Common.h"
#include "ImageUtils.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"
#include "Log.h"
#include "Random.h"
#include "ComponentIconRenderer.h"

#include <vector>
#include <set>
#include <fstream>

void CSceneGoldSrcRenderPass::_loadSceneV(ptr<SSceneInfoGoldSrc> vScene)
{
    m_pDevice->waitUntilIdle();
    m_CurTextureIndex = 0;

    __createVertexBuffer();
    __destroySceneResources();
    __createSceneResources();
    __updateTextureView();
}

void CSceneGoldSrcRenderPass::rerecordAllCommand()
{
    m_pRerecord->requestRecordForAll();
}

void CSceneGoldSrcRenderPass::_initV()
{
    CRenderPassScene::_initV();

    m_pRerecord = make<CRerecordState>(m_pAppInfo);
    m_pRerecord->addField("Primary");
    for (const auto& Name : _getExtraCommandBufferNamesV())
        m_pRerecord->addField(Name);
    
    VkExtent2D RefExtent = { 0, 0 };
    _dumpReferenceExtentV(RefExtent);

    m_DepthImageManager.init(RefExtent, false, 
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            vImageSet.init(1);
            VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
            ImageUtils::createDepthImage(*vImageSet[0], m_pDevice, vExtent, NULL, DepthFormat);
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
            m_pRerecord->requestRecord("Mesh");
        }
    };

    m_PipelineSet.Normal.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendTextureAlpha.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendAlphaTest.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);
    m_PipelineSet.BlendAdditive.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), CreateGoldSrcPipelineFunc);

    m_PipelineSet.Simple.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](CPipelineSimple& vPipeline)
        {
            if (vPipeline.isValid())
            {
                __updatePipelineResourceSimple(vPipeline);
                m_pRerecord->requestRecord("Mesh");
            }
        });
    m_PipelineSet.Sky.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](CPipelineSkybox& vPipeline)
        {
            if (vPipeline.isValid())
            {
                __updatePipelineResourceSky(vPipeline);
                m_pRerecord->requestRecord("Sky");
            }
        });
    m_PipelineSet.Sprite.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](CPipelineSprite& vPipeline)
        {
            if (vPipeline.isValid())
            {
                __updatePipelineResourceSprite(vPipeline);
                m_pRerecord->requestRecord("Sprite");
            }
        });
    m_PipelineSet.Icon.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](IPipeline& vPipeline)
        {
            m_pRerecord->requestRecord("Icon");
        });
    m_PipelineSet.Text.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(),
        [this](IPipeline& vPipeline)
        {
            m_pRerecord->requestRecord("Text");
        });

    __createSceneResources();

    rerecordAllCommand();
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

void CSceneGoldSrcRenderPass::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CSceneGoldSrcRenderPass::_renderUIV()
{
    if (UI::collapse(u8"��Ⱦ����"))
    {
        static const std::vector<const char*> RenderMethodNames =
        {
            u8"��Դ��Ⱦ",
            u8"����",
        };
        
        int RenderMethodIndex = static_cast<int>(m_RenderMethod);
        if (UI::combo(u8"��Ⱦ��", RenderMethodNames, RenderMethodIndex))
        {
            setRenderMethod(static_cast<ERenderMethod>(RenderMethodIndex));
        }

        bool SkyRendering = getSkyState();
        UI::toggle(u8"���������Ⱦ", SkyRendering);
        setSkyState(SkyRendering);
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

void CSceneGoldSrcRenderPass::_destroyV()
{
    m_DepthImageManager.destroy();
    m_PipelineSet.destroy();
    m_pVertexBuffer->destroy();

    __destroySceneResources();

    CRenderPassScene::_destroyV();
}

std::vector<VkCommandBuffer> CSceneGoldSrcRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
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
            m_PipelineSet.Sky.get().recordCommand(pSkyCmdBuffer, vImageIndex);
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
                m_PipelineSet.Normal.get().bind(pMeshCmdBuffer, vImageIndex);
                m_PipelineSet.Normal.get().setOpacity(pMeshCmdBuffer, 1.0f);
            }
            else if (m_RenderMethod == ERenderMethod::SIMPLE)
            {
                m_PipelineSet.Simple.get().bind(pMeshCmdBuffer, vImageIndex);
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
                    m_PipelineSet.Normal.get().setLightmapState(pMeshCmdBuffer, EnableLightmap);
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
            m_PipelineSet.Sprite.get().recordCommand(pSpriteCmdBuffer, vImageIndex);
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
            auto& PipelineIcon = m_PipelineSet.Icon.get();
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
    auto& PipelineText = m_PipelineSet.Text.get();
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
    if (m_PipelineSet.Simple.isReady())
        __updatePipelineResourceSimple(m_PipelineSet.Simple.get());
    if (m_PipelineSet.Sky.isReady())
        __updatePipelineResourceSky(m_PipelineSet.Sky.get());
    if (m_PipelineSet.Sprite.isReady())
        __updatePipelineResourceSprite(m_PipelineSet.Sprite.get());

    // search all text renderer
    if (m_pSceneInfo)
    {
        auto& PipelineText = m_PipelineSet.Text.get();
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

void CSceneGoldSrcRenderPass::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();
    m_LightmapImage.destroy();
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
//        pPipeline = &m_PipelineSet.Normal.get();
//        break;
//    }
//    case EGoldSrcRenderMode::COLOR:
//    case EGoldSrcRenderMode::TEXTURE:
//    {
//        pPipeline = &m_PipelineSet.BlendTextureAlpha.get();
//        break;
//    }
//    case EGoldSrcRenderMode::SOLID:
//    {
//        pPipeline = &m_PipelineSet.BlendAlphaTest.get();
//        break;
//    }
//    case EGoldSrcRenderMode::ADDITIVE:
//    case EGoldSrcRenderMode::GLOW:
//    {
//        pPipeline = &m_PipelineSet.BlendAdditive.get();
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

void CSceneGoldSrcRenderPass::__createTextureImages()
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

void CSceneGoldSrcRenderPass::__createLightmapImage()
{
    if (m_pSceneInfo && m_pSceneInfo->UseLightmap)
    {
        ptr<CIOImage> pCombinedLightmapImage = m_pSceneInfo->pLightmap->getCombinedLightmap();
        ImageUtils::createImageFromIOImage(m_LightmapImage, m_pDevice, pCombinedLightmapImage);
    }
}

void CSceneGoldSrcRenderPass::__createVertexBuffer()
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
        Log::log("����: �������� = (" + std::to_string(NumTexture) + ") ������������ (" + std::to_string(CPipelineNormal::MaxTextureNum) + "), ���������������");
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

void CSceneGoldSrcRenderPass::__updatePipelineResourceSimple(CPipelineSimple& vPipeline)
{
    vPipeline.setTextures(m_TextureImageSet);
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

void CSceneGoldSrcRenderPass::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Normal.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendTextureAlpha.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendAlphaTest.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.BlendAdditive.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.Simple.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    m_PipelineSet.Sprite.get().updateUniformBuffer(vImageIndex, m_pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.get().updateUniformBuffer(vImageIndex, m_pCamera);
    m_PipelineSet.Icon.get().updateUniformBuffer(vImageIndex, m_pCamera);
    m_PipelineSet.Text.get().updateUniformBuffer(vImageIndex, m_pCamera);
}
