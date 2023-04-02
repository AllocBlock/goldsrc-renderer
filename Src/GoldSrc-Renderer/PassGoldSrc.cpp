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
    if (UI::collapse(u8"仿金源"))
    {
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
    
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);

    bool RerecordCommand = false;
    if (m_RerecordCommandTimes > 0)
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

        // 1. sky
        if (m_EnableSky)
        {
            m_PipelineSet.Sky.get().recordCommand(CommandBuffer, vImageIndex);
        }

        // 2. scene mesh
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
            m_PipelineSet.Normal.get().bind(CommandBuffer, vImageIndex);
            m_PipelineSet.Normal.get().setOpacity(CommandBuffer, 1.0f);

            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);
                if (!pActor->getVisible()) continue;

                auto pTransform = pActor->getTransform();
                auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
                if (!pMeshRenderer) continue;

                auto pMesh = pMeshRenderer->getMesh();
                if (!pMesh) continue;

                bool EnableLightmap = pMesh->getMeshDataV().hasLightmap();
                m_PipelineSet.Normal.get().setLightmapState(CommandBuffer, EnableLightmap);
                __drawActor(vImageIndex, pActor);
            }
        }

        // 3. sprite and icon
        if (m_pSceneInfo && !m_pSceneInfo->SprSet.empty())
        {
            m_PipelineSet.Sprite.get().recordCommand(CommandBuffer, vImageIndex);
        }

        if (m_pSceneInfo)
        {
            auto& PipelineIcon = m_PipelineSet.Icon.get();
            PipelineIcon.clear();
            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                auto pActor = m_pSceneInfo->pScene->getActor(i);
                if (!pActor->getVisible()) continue;

                auto pTransform = pActor->getTransform();
                auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
                if (pMeshRenderer) continue;
                // TODO: change to find if there is icon renderer!

                glm::vec3 Position = pActor->getTransform()->getAbsoluteTranslate();
                PipelineIcon.addIcon(EIcon::TIP, Position);
            }
            PipelineIcon.recordCommand(CommandBuffer, vImageIndex);
        }

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

void CSceneGoldSrcRenderPass::__drawActor(uint32_t vImageIndex, CActor::Ptr vActor)
{
    VkCommandBuffer CommandBuffer = _getCommandBuffer(vImageIndex);
    _drawActor(CommandBuffer, vActor);
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
    m_PipelineSet.Icon.get().updateUniformBuffer(vImageIndex, m_pCamera);
}
